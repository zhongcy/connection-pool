/* Copyright 2013 Active911 Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http: *www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


/* ConnectionPool manages a connection pool of some kind.  Worker threads can ask for a connection, and must return it when done.
 * Each connection is guaranteed to be healthy, happy, and free of disease.
 *
 * Connection and ConnectionFactory are virtual classes that should be overridden to their actual type.
 *
 * NOTE: To avoid using templates AND inheritance at the same time in the ConnectionFactory, ConnectionFactory::create must create a derved type 
 * but return the base class. 	
 */

#pragma once

// Define your custom logging function by overriding this #define
#ifndef _DEBUG
	#define _DEBUG(x)
#else
	#define _DEBUG_MODE
#endif



#include <deque>
#include <set>
#include <exception>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/format.hpp>

namespace active911 {


	struct ConnectionUnavailable : std::exception { 

		char const* what() const throw() {

			return "Unable to allocate connection";
		}; 
	};


	class Connection {

	public:
		Connection(){};
		virtual ~Connection(){};
		virtual bool isValid() = 0;
        virtual bool reconnect() = 0;
	};

	class ConnectionFactory {

	public:
		virtual boost::shared_ptr<Connection> create()=0;
		virtual ~ConnectionFactory(){};
	};

	struct ConnectionPoolStats {

		size_t pool_size;
		size_t borrowed_size;
		int retry_cnt;
		int timeout_sec;

	};

	template<class T>
	class ConnectionPool {

	public:

		ConnectionPoolStats get_stats() {

			// Lock
			boost::mutex::scoped_lock lock(this->io_mutex);

			// Get stats
			ConnectionPoolStats stats;
			stats.pool_size=this->pool.size();
			stats.borrowed_size=this->borrowed.size();			

			// timeout
			stats.timeout_sec=this->timeout_sec;

			return stats;
		};

		ConnectionPool(size_t pool_size, boost::shared_ptr<ConnectionFactory> factory, unsigned int timeout_sec=0){

			// Setup
			this->pool_size=pool_size;
			this->factory=factory;

			// Fill the pool
			while(this->pool.size() < this->pool_size){

				this->pool.push_back(this->factory->create());
			}

			// timeout
			this->timeout_sec=timeout_sec;

			// timelock
			pthread_mutex_init(&timelock_mutex, NULL);
			pthread_cond_init(&timelock_cond, NULL);

		};


		~ConnectionPool() {

			// timelock
			pthread_cond_destroy(&timelock_cond);
			pthread_mutex_destroy(&timelock_mutex);

		};


		/**
		 * Borrow
		 *
		 * Borrow a connection for temporary use
		 *
		 * When done, either (a) call unborrow() to return it, or (b) (if it's bad) just let it go out of scope.  This will cause it to automatically be replaced.
		 * @retval a shared_ptr to the connection object
		 */
		boost::shared_ptr<T> borrow(){

			// Timelock
			struct timeval now;
			struct timespec ts;
			gettimeofday(&now, NULL);
			ts.tv_sec = now.tv_sec + timeout_sec;
			ts.tv_nsec = now.tv_usec * 1000;
			int64_t lock_usec=ts.tv_sec*1000000+now.tv_usec;
			int64_t now_usec;

			do{

				// Lock
				boost::mutex::scoped_lock lock(this->io_mutex);

				if(pool.empty()) {//fail

					// Are there any crashed connections listed as "borrowed"?
					for(std::set<boost::shared_ptr<Connection> >::iterator it=this->borrowed.begin(); it!=this->borrowed.end(); ++it){

						if((*it).unique()) {

							// This connection has been abandoned! Destroy it and create a new connection
							try {

								// If we are able to create a new connection, return it
								_DEBUG("Creating new connection to replace discarded connection");
								boost::shared_ptr<Connection> conn=this->factory->create();
								this->borrowed.erase(it);
								this->borrowed.insert(conn);
								return boost::static_pointer_cast<T>(conn);

							} catch(std::exception& e) {

								break;

							}
						}
					}

					// Timelock
					gettimeofday(&now, NULL);
					now_usec=now.tv_sec*1000000+now.tv_usec;
					if(lock_usec>now_usec){

#ifdef _DEBUG_MODE
						{
							char tmp[128];
							time_t now_time=time(NULL);
							struct tm *t=localtime(&now_time);
							snprintf(tmp,128,"[%02d:%02d:%02d] time lock tid:%x",
									 t->tm_hour,t->tm_min,t->tm_sec,(unsigned int)pthread_self());
							_DEBUG(tmp);
						}
#endif
						lock.unlock();
						pthread_mutex_lock(&timelock_mutex);
						pthread_cond_timedwait(&timelock_cond, &timelock_mutex, &ts);
						pthread_mutex_unlock(&timelock_mutex);

#ifdef _DEBUG_MODE
						{
							char tmp[128];
							time_t now_time=time(NULL);
							struct tm *t=localtime(&now_time);
							snprintf(tmp,128,"[%02d:%02d:%02d] time unlock pid:%x",
									 t->tm_hour,t->tm_min,t->tm_sec,(unsigned int)pthread_self());
							_DEBUG(tmp);
						}
#endif

					}

				}else{

					// Take one off the front
					boost::shared_ptr<Connection> conn = this->pool.front();
					this->pool.pop_front();

					if(false == conn->isValid())
                    {
                        if(false == conn->reconnect())
                        {
                            throw ConnectionUnavailable();
                        }
                    }

					// Add it to the borrowed list
					this->borrowed.insert(conn);

					return boost::static_pointer_cast<T>(conn);

				}

				gettimeofday(&now, NULL);
				now_usec=now.tv_sec*1000000+now.tv_usec;
			}while(lock_usec>now_usec);

			throw ConnectionUnavailable();

		};

		/**
		 * Unborrow a connection
		 *
		 * Only call this if you are returning a working connection.  If the connection was bad, just let it go out of scope (so the connection manager can replace it).
		 * @param the connection
		 */
		void unborrow(boost::shared_ptr<T> conn) {

			// Lock
			boost::mutex::scoped_lock lock(this->io_mutex);

			// Push onto the pool
			this->pool.push_back(boost::static_pointer_cast<Connection>(conn));

			// Unborrow
			this->borrowed.erase(conn);

			// Timelock
			pthread_cond_signal(&timelock_cond);

		};

	protected:
		boost::shared_ptr<ConnectionFactory> factory;
		size_t pool_size;
		std::deque<boost::shared_ptr<Connection> > pool;
		std::set<boost::shared_ptr<Connection> > borrowed;
		boost::mutex io_mutex;
		unsigned int timeout_sec;

		pthread_cond_t timelock_cond;
		pthread_mutex_t timelock_mutex;
	};

}
