#include "WorldSession.h"
#include "NECROWorld.h"
#include "CharacterData.h"
#include "WorldCmdTypes.h"

#include <boost/asio.hpp>
#include <memory>

namespace NECRO
{
namespace World
{
	bool WorldSession::Handle_SPacketEnterWorld()
	{
		// Fixed size packet
		if (m_currentDecryptedPacket.GetActiveSize() != sizeof(SPacketEnterWorld))
			return false;

		m_lastActivity = std::chrono::steady_clock::now();

		SPacketEnterWorld* pckt = reinterpret_cast<SPacketEnterWorld*>(m_currentDecryptedPacket.GetReadPointer());

		// Make sure that charID is associated with the accountid that originated the request
        std::shared_ptr<EnterWorldCtx> reqCtx = std::make_shared<EnterWorldCtx>(EnterWorldCtx({pckt->characterID}));
        auto& dbworker = Server::Instance().GetCharactersDBPool();
        {
            DBRequest req(m_ioContextRef, false);
            req.m_steps.push_back({ static_cast<uint32_t>(CharactersDatabaseStatements::CHAR_SEL_BY_CHARID), {reqCtx->characterID } }); // check for characters limit on the account

            // The callback needs to ensure the object still exists, as it may be deleted by the main thread while the dbrequest is being processed
            std::weak_ptr<WorldSession> weakSelf = shared_from_this();
            req.m_callback = [weakSelf, reqCtx](uint32_t ec, std::vector<mysqlx::SqlResult>& res)
                {
                    if (auto lockedSelf = weakSelf.lock())
                        return lockedSelf->DBCallback_HandleEnterWorldChecks(ec, res, reqCtx);

                    return false; // WorldSession is destroyed (disconnect)
                };

            req.m_cancelToken = weakSelf;

            if (!dbworker.TryEnqueue(std::move(req)))
                return false; // TODO, we could return a SERVER_BUSY and not drop the connection, but just cancel the current action if possible
        }

        m_status = WorldSocketStatus::ENTERING_WORLD;

		return true;
	}

	bool WorldSession::DBCallback_HandleEnterWorldChecks(uint32_t ec, std::vector<mysqlx::SqlResult>& result, std::shared_ptr<EnterWorldCtx> reqCtx)
	{
        if (!IsOpen())
            return false;

        if (ec != 0)
        {
            LOG_DEBUG("DBCallback_HandleEnterWorldChecks's query returned an error. There's no way to continue. Dropping socket.");
            CloseSocket();
            return false;
        }

        LOG_DEBUG("Handling DBCallback_HandleEnterWorldChecks for user {}!", m_data.accountID);

        m_lastActivity = std::chrono::steady_clock::now();

        Packet p;
        p << static_cast<uint16_t>(World::PacketIDs::ENTER_WORLD);

        // Check the callback
        // If CharID doesnt exist.
        if (result[0].count() <= 0)
        {
            // Send error message to the client, but don't drop him
            p << static_cast<uint8_t>(World::WorldResults::CHARACTER_NOT_FOUND);
            NetworkMessage m(std::move(p));
            int encryptRes = m.AESEncrypt(m_data.sessionKey.data(), m_data.iv, nullptr, 0);
            if (encryptRes < 0)
            {
                LOG_ERROR("Failed to encrypt packet, returned {}. Dropping the connection.", encryptRes);
                CloseSocket();
                return false;
            }
            QueuePacket(std::move(m));

            m_status = WorldSocketStatus::AUTHED;

            return true;
        }
        else
        {
            mysqlx::Row row = result[0].fetchOne(); //result[0] is result of m_step[0]

            // Check the account id of the row that returned
            uint32_t dbAccountID = row[0].get<uint32_t>();

            if (dbAccountID == m_data.accountID)
            {
                // Proceed, spawn the character in the world
                std::weak_ptr<WorldSession> weakSelf = shared_from_this();

                // Fill the remaining data from the DBRow
                std::shared_ptr<NECRO::CharacterData> characterData = std::make_shared<NECRO::CharacterData>();
                characterData->id = reqCtx->characterID;
                characterData->characterName = row[1].get<std::string>();
                characterData->characterNameLength = characterData->characterName.length();
                characterData->race     = static_cast<uint8_t>(row[2].get<int>());
                characterData->gameClass = static_cast<uint8_t>(row[3].get<int>());
                characterData->gender   = static_cast<uint8_t>(row[4].get<int>());
                characterData->level    = static_cast<uint8_t>(row[5].get<int>());
                characterData->xp       = static_cast<uint32_t>(row[6].get<uint32_t>());
                characterData->zone     = static_cast<uint32_t>(row[7].get<uint32_t>());
                characterData->pos_x    = static_cast<float_t>(row[8].get<float_t>());
                characterData->pos_y    = static_cast<float_t>(row[9].get<float_t>());
                characterData->pos_z    = static_cast<float_t>(row[10].get<float_t>());

                boost::asio::io_context* originatingIoContext = &m_ioContextRef;
                Server::Instance().GetWorldSimulation().PostWorldCmd(
                    [weakSelf, characterData, originatingIoContext]()
                    {
                        // THIS RUNS ON THE MAIN (SIMULATION) THREAD!
                        
                        // If sesion died
                        if (weakSelf.expired())
                            return;

                        PlayerSpawnCmdResult spawnResult = Server::Instance().GetWorldSimulation().WorldCmd_TryToSpawnPlayerCharacter(*characterData);

                        // TODO: fix the edge case where the world session dies between the already executed spawn and the callback we're posting below
                        // WorldSession destructor could despawn it if we set the guid earlier than now (WorldCmdCallback_OnEnterWorld)

                        // Post result on the originating NetworkThread context
                        boost::asio::post(*originatingIoContext,
                            [weakSelf, spawnResult]()
                            {
                                if (auto s = weakSelf.lock())
                                    s->WorldCmdCallback_OnEnterWorld(spawnResult);
                            });
                    });
            }
            else
            {
                // Send error message to the client, but don't drop him (although this happening is sketchy)
                p << static_cast<uint8_t>(World::WorldResults::CHARACTER_DOES_NOT_BELONG_TO_ACCOUNT);
                NetworkMessage m(std::move(p));
                int encryptRes = m.AESEncrypt(m_data.sessionKey.data(), m_data.iv, nullptr, 0);
                if (encryptRes < 0)
                {
                    LOG_ERROR("Failed to encrypt packet, returned {}. Dropping the connection.", encryptRes);
                    CloseSocket();
                    return false;
                }
                QueuePacket(std::move(m));

                m_status = WorldSocketStatus::AUTHED;

                return true;
            }
        }

        return true;
	}

    bool WorldSession::WorldCmdCallback_OnEnterWorld(PlayerSpawnCmdResult result)
    {
        if (!IsOpen())
            return false;

        LOG_DEBUG("Handling WorldCmdCallback_OnEnterWorld for user {}!", m_data.accountID);

        m_lastActivity = std::chrono::steady_clock::now();

        // Send response
        Packet p;
        p << static_cast<uint16_t>(World::PacketIDs::ENTER_WORLD);
        if (result.success)
        {
            // Set our stuff
            m_playerGUID = result.guid;
            m_playerPtr = result.playerPtr;
            
            p << static_cast<uint8_t>(World::WorldResults::SUCCESS);
            p << static_cast<uint64_t>(result.guid);
            p << static_cast<uint32_t>(result.mapID);
            p << static_cast<float_t>(result.posX);
            p << static_cast<float_t>(result.posY);
           
            m_status = WorldSocketStatus::IN_WORLD;
        }
        else
        {
            p << static_cast<uint8_t>(World::WorldResults::FAILED);
            m_status = WorldSocketStatus::AUTHED;
        }

        NetworkMessage m(std::move(p));
        int encryptRes = m.AESEncrypt(m_data.sessionKey.data(), m_data.iv, nullptr, 0);
        if (encryptRes < 0)
        {
            LOG_ERROR("Failed to encrypt packet, returned {}. Dropping the connection.", encryptRes);
            CloseSocket();
            return false;
        }
        QueuePacket(std::move(m));
        return true;
    }
}
}
