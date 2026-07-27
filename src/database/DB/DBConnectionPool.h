#pragma once

#include <string>
#include <memory>
#include <mysqlx/xdevapi.h>

#include "Logger.h"
#include "FileLogger.h"
#include "ConsoleLogger.h"

namespace NECRO
{
    inline constexpr int DB_CONN_POOL_MAX_SIZE = 100;

    class DBConnectionPool
    {
    public:
        std::unique_ptr<mysqlx::Client> m_client;

        int Init(const std::string& URI)
        {
            try
            {
                std::string uri = "mysqlx://" + URI;
                m_client = std::make_unique<mysqlx::Client>(uri, mysqlx::ClientOption::POOLING, false); // Pooling is disabled as it had something to do with some previous issues that are now fixed, need to test again
                LOG_INFO("DBConnectionPool initialized successfully.");
                return 0;
            }
            catch (const mysqlx::Error& err)
            {
                LOG_INFO(std::string("Error initializing DBConnectionPool: ") + err.what());
                return -1;
            }
            catch (std::exception& ex)
            {
                LOG_INFO(std::string("Standard exception during DBConnectionPool initialization: ") + ex.what());
                return -2;
            }
            catch (...)
            {
                LOG_INFO(std::string("Unknown exception during DBConnectionPool initialization."));
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
                LOG_INFO(std::string("DBConnectionPool Close Error: : ") + err.what());
            }
            catch (std::exception& ex)
            {
                LOG_INFO(std::string("DBConnectionPool Close Error: Standard exception: ") + ex.what());
            }
            catch (...)
            {
                LOG_INFO(std::string("DBConnectionPool Close Error: Unknown exception during session initialization."));
            }
        }
    };
}
