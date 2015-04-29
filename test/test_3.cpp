// Log to stdout
#define _DEBUG(x) 										\
	do { 												\
		std::cout << "  (" << x << ")" << std::endl;	\
	} while(0)

#include <iostream>
#include <string>
#include <cassert>
#include <pthread.h>
#include <boost/shared_ptr.hpp>
#include "../DummyConnection.h"

using namespace active911;
using namespace std;

static bool exception_thrown=false;
static boost::shared_ptr<ConnectionPool<DummyConnection> >pool;

void *getConnection(void *) {

	try{

		boost::shared_ptr<DummyConnection> conn = pool->borrow();
		sleep(1);
		pool->unborrow(conn);

	}catch(std::exception& e){

		cout << "Exception thrown (intentional)" << endl;
		exception_thrown=true;

	}
	return NULL;

}

int main(int argc, char **argv) {

	// Create a pool of 2 dummy connections
	cout << "Creating connections..." << endl;
	boost::shared_ptr<DummyConnectionFactory>connection_factory(new DummyConnectionFactory());
	pool = boost::shared_ptr<ConnectionPool<DummyConnection> >(new ConnectionPool<DummyConnection>(2, connection_factory,10));
	ConnectionPoolStats stats=pool->get_stats();
	assert(stats.pool_size==2);

	{

		// pool num: 2, timeout: 10, thread 20: test success
		exception_thrown=false;
		cout << "pool num: 2, timeout: 10, thread 20: test success" << endl;
		const int thread_num=20;
		pthread_t th[thread_num];
		for(int i=0;i<thread_num;i++){

			pthread_create(&th[i], NULL, getConnection, NULL);

		}
		for(int i=0;i<thread_num;i++){

			pthread_join(th[i], NULL);

		}
		assert(!exception_thrown);

	}

	// thread 21~22: sometime success, sometime fail. not test.

	{

		// pool num: 2, timeout: 10, thread 23: test fail
		exception_thrown=false;
		cout << "pool num: 2, timeout: 10, thread 23: test fail" << endl;
		const int thread_num = 23;
		pthread_t th[thread_num];
		for(int i=0;i<thread_num;i++){

			pthread_create(&th[i], NULL, getConnection, NULL);

		}
		for(int i=0;i<thread_num;i++){

			pthread_join(th[i], NULL);

		}
		assert(exception_thrown);

	}

	return 0;
}

