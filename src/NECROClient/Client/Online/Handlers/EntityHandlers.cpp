#include "WorldSession.h"
#include "WorldCodes.h"
#include "NECROEngine.h"
#include "Player.h"

namespace NECRO
{
namespace Client
{
	bool WorldSession::Handle_EntitySpawn()
	{
		if (!IsOpen())
			return false;

		LOG_DEBUG("Handle_EntitySpawn!");

		NECRO::World::CPacketEntitySpawn* pckt = reinterpret_cast<NECRO::World::CPacketEntitySpawn*>(m_currentDecryptedPacket.GetReadPointer());

		// Check for packet size and fields
		if (m_currentDecryptedPacket.GetActiveSize() < (sizeof(NECRO::World::CPacketEntitySpawn) - 1))
			return false;

		if (pckt->nameLength > NECRO::World::CHARACTER_MAX_NAME_LENGTH || m_currentDecryptedPacket.GetActiveSize() != (sizeof(NECRO::World::CPacketEntitySpawn) - 1) + pckt->nameLength)
			return false;

		if (pckt->direction >= ISO_DIRECTIONS_N)
			return false;

		World* world = engine.GetGame().GetCurrentWorld();

		// If the entity was already known, the spawn just refreshes its position (only players are network entities for now)
		Entity* existing = world->GetNetworkEntity(pckt->guid);
		if (existing)
		{
			static_cast<Player*>(existing)->SetNetworkTarget(pckt->pos_x, pckt->pos_y, pckt->pos_z, static_cast<IsoDirection>(pckt->direction));
			return true;
		}

		// Only players for now, TODO
		if (pckt->entityType != static_cast<uint8_t>(NECRO::World::EntityType::PLAYER_ENTITY))
		{
			LOG_WARNING("Handle_EntitySpawn - EntityType '{}' is not supported yet, ignoring it.", pckt->entityType);
			return true;
		}

		// TODO set the image based on the class/gender
		std::unique_ptr<Player> p = std::make_unique<Player>(true);
		p->SetImg(engine.GetAssetsManager().GetImage("player_war_idle.png"));
		p->m_pos = Vector2(pckt->pos_x, pckt->pos_y);
		p->m_zPos = pckt->pos_z;
		p->SetNetworkTarget(pckt->pos_x, pckt->pos_y, pckt->pos_z, static_cast<IsoDirection>(pckt->direction));
		p->SetLayer(0);
		p->Init();
		p->SetFlag(Entity::Flags::FDynamic);
		p->SetName(std::string(reinterpret_cast<const char*>(pckt->name), pckt->nameLength));

		if (!world->AddNetworkEntity(pckt->guid, std::move(p)))
			LOG_WARNING("Handle_EntitySpawn - Could not add entity GUID: '{}' at ({}, {}), it's probably out of bounds.", pckt->guid, pckt->pos_x, pckt->pos_y);

		return true;
	}

	bool WorldSession::Handle_EntityDespawn()
	{
		if (!IsOpen())
			return false;

		LOG_DEBUG("Handle_EntityDespawn!");

		// Fixed size packet
		if (m_currentDecryptedPacket.GetActiveSize() != sizeof(NECRO::World::CPacketEntityDespawn))
			return false;

		NECRO::World::CPacketEntityDespawn* pckt = reinterpret_cast<NECRO::World::CPacketEntityDespawn*>(m_currentDecryptedPacket.GetReadPointer());

		engine.GetGame().GetCurrentWorld()->RemoveNetworkEntity(pckt->guid);
		return true;
	}

	bool WorldSession::Handle_EntityMovementUpdate()
	{
		if (!IsOpen())
			return false;

		// Fixed size packet
		if (m_currentDecryptedPacket.GetActiveSize() != sizeof(NECRO::World::CPacketEntityMovementUpdate))
			return false;

		NECRO::World::CPacketEntityMovementUpdate* pckt = reinterpret_cast<NECRO::World::CPacketEntityMovementUpdate*>(m_currentDecryptedPacket.GetReadPointer());

		if (pckt->direction >= ISO_DIRECTIONS_N)
			return false;

		// Unknown GUIDs are ignored. TODO: should think about this, maybe we should request a spawn for the entity if it's unknown and it keeps receiving updates from the server.
		Entity* e = engine.GetGame().GetCurrentWorld()->GetNetworkEntity(pckt->guid);
		if (e)
			static_cast<Player*>(e)->SetNetworkTarget(pckt->pos_x, pckt->pos_y, pckt->pos_z, static_cast<IsoDirection>(pckt->direction)); // only players are network entities for now

		return true;
	}
}
}
