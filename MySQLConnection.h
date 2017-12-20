/* Copyright 2013 Active911 Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	http: *www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "ConnectionPool.h"
#include <string>
#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/statement.h>

namespace active911 {


	class MySQLConnection : public Connection {

	public:


		~MySQLConnection() {

			if(this->sql_connection) {

				_DEBUG("MYSQL Destruct");

				this->sql_connection->close();
				this->sql_connection.reset(); 	// Release and destruct
				
			}

		};

		bool isValid()
		{
			return this->sql_connection->isValid();
		}

		bool reconnect()
		{
			return this->sql_connection->reconnect();
		}

		boost::shared_ptr<sql::Connection> sql_connection;
	};


	class MySQLConnectionFactory : public ConnectionFactory {

	public:
		MySQLConnectionFactory(std::string server,
                               std::string username,
                               std::string password,
                               int connect_timeout_sec = 0,
                               int read_timeout_sec = 0,
                               int write_timeout_sec = 0) {

			this->server=server;
			this->username=username;
			this->password=password;
            this->connect_timeout_sec = connect_timeout_sec;
            this->read_timeout_sec = read_timeout_sec;
            this->write_timeout_sec = write_timeout_sec;

		};

		// Any exceptions thrown here should be caught elsewhere
		boost::shared_ptr<Connection> create() {

			// Get the driver
			sql::Driver *driver;
			driver=get_driver_instance();

			// Create the connection
			boost::shared_ptr<MySQLConnection>conn(new MySQLConnection());

            // Set connection properties
            sql::ConnectOptionsMap connection_properties;
            connection_properties["hostName"] = server;
            connection_properties["userName"] = username;
            connection_properties["password"] = password;
            if(connect_timeout_sec > 0){
                connection_properties["OPT_CONNECT_TIMEOUT"] = connect_timeout_sec;
            }
            if(read_timeout_sec > 0){
                connection_properties["OPT_READ_TIMEOUT"] = read_timeout_sec;
            }
            if(write_timeout_sec > 0){
                connection_properties["OPT_WRITE_TIMEOUT"] = write_timeout_sec;
            }
			// Connect
			conn->sql_connection=boost::shared_ptr<sql::Connection>(driver->connect(connection_properties));

			return boost::static_pointer_cast<Connection>(conn);
		};

	private:
		std::string server;
		std::string username;
		std::string password;
        int connect_timeout_sec;
        int read_timeout_sec;
        int write_timeout_sec;
	};





}
