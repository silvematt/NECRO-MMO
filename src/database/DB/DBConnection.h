#ifndef NECRO_DBCONNECTION_H
#define NECRO_DBCONNECTION_H

#include <string>
#include <memory>
#include <mysqlx/xdevapi.h>

#include "Logger.h"
#include "FileLogger.h"
#include "ConsoleLogger.h"

namespace NECRO
{
    //-------------------------------------------------------
    // Abstracts mysqlx::Session
    //-------------------------------------------------------
    class DBConnection
    {
    public:
        std::unique_ptr<mysqlx::Session> m_session;

        int Init(const std::string& host, int port, const std::string& user, const std::string& pass)
        {
            try
            {
                // Establish a session to the MySQL server
                m_session = std::make_unique<mysqlx::Session>(host, port, user, pass);
                LOG_INFO("Database session initialized successfully.");
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
                m_session->close();
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
