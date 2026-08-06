#pragma once

#include <cstdint>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>

#include "ConsoleLogger.h"
#include "FileLogger.h"
#include "WorldSession.h"

namespace NECRO
{
namespace World
{
    // Session slots are identified by a serial to keep track of the owner of that slot
    struct SessionSlot 
    { 
        uint64_t serial; 
        std::weak_ptr<WorldSession> session; 
    };

	// -------------------------------------------------------------------------------------------
	// Keeps track of all the WorldSessions that have been authenticated, indexed by account id
	// -------------------------------------------------------------------------------------------
	class SessionManager
	{
	private:
		std::mutex m_sessionMutex;

        // AccountID -> Session
		std::unordered_map<uint32_t, SessionSlot> m_sessions;

        std::atomic<uint32_t> m_playerCount{0};

	public:
        bool RegisterSession(uint32_t accountID, const std::shared_ptr<WorldSession>& session)
        {
            if (!session)
                return false;

            const uint64_t serial = session->GetSessionSerial();
            std::shared_ptr<WorldSession> toKick;

            {
                std::lock_guard lock(m_sessionMutex);

                auto it = m_sessions.find(accountID);
                if (it != m_sessions.end())
                {
                    LOG_DEBUG("Kicking previous logged session of AccountID: '{}'", accountID);

                    toKick = it->second.session.lock();      // may be null if already dead
                    if (toKick)
                        toKick->PostForceCloseSocket();

                    m_sessions.erase(it);
                    m_playerCount.fetch_sub(1, std::memory_order_relaxed);
                }

                // Add new session
                m_sessions.insert_or_assign(accountID, SessionSlot{ serial, session });
                m_playerCount.fetch_add(1, std::memory_order_relaxed);
            }

            return true;
        }

        bool UnregisterSession(uint32_t accountID, uint64_t serial)
        {
            std::lock_guard lock(m_sessionMutex);
            auto it = m_sessions.find(accountID);
            if (it == m_sessions.end() || it->second.serial != serial) // If the serials are not the same, the slot now belongs to a newer session and this was called by the old destructor, so do nothing
                return false; 

            m_sessions.erase(it);
            m_playerCount.fetch_sub(1, std::memory_order_relaxed);
            return true;
        }
	};
}
}
