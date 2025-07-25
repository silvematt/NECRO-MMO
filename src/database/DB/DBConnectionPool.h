#ifndef NECRO_DBCONNECTIONPOOL_H
#define NECRO_DBCONNECTIONPOOL_H

#include <string>
#include <memory>
#include <mysqlx/xdevapi.h>

#include "Logger.h"
#include "FileLogger.h"
#include "ConsoleLogger.h"


namespace NECRO
{
    class DBConnectionPool
    {
    public:
        std::unique_ptr<mysqlx::Client> m_client;

        int Init(const std::string& host, int port, const std::string& user, const std::string& pass, const std::string& dbname)
        {
            try
            {
                std::string uri = "mysqlx://" + user + ":" + pass + "@" + host + "/" + dbname;

                m_client = std::make_unique<mysqlx::Client>(uri, mysqlx::ClientOption::POOLING, false); // POOLING = true crashes the mysqlx library itself (?)
                LOG_INFO("DBConnectionPool initialized successfully.");
                return 0;
            }
            catch (const mysqlx::Error& err)
            {
                LOG_INFO(std::string("Error initializing session: ") + err.what());
                return -1;
            }
            catch (std::exception& ex)
            {
                LOG_INFO(std::string("Standard exception: ") + ex.what());
                return -2;
            }
            catch (...)
            {
                LOG_INFO(std::string("Unknown exception during session initialization."));
                return -3;
            }

            return -4;
        }

        void Close()
        {
            try
            {
                m_client->close();
            }
            catch (const mysqlx::Error& err)
            {
                LOG_INFO(std::string("DBConnection Close Error: : ") + err.what());
            }
            catch (std::exception& ex)
            {
                LOG_INFO(std::string("DBConnection Close Error: Standard exception: ") + ex.what());
            }
            catch (...)
            {
                LOG_INFO(std::string("DBConnection Close Error: Unknown exception during session initialization."));
            }
        }
    };

}

#endif
