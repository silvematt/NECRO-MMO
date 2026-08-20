#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <stdexcept>

#include "MapDef.h"
#include "Cell.h"
#include "Entity.h"

namespace NECRO
{
namespace World
{
	// ---------------------------------------------------------------------------------------------------------------------------
	// A loaded map in the Server is called a "Zone". It can be an exterior, a dungeon or anything in between.
	// A Zone is an entry in the std::unordered_map<uint32_t, std::unique_ptr<Zone>>	m_zones and it represents a loaded Map.
	// 
	// Will always be child of the WorldSimulation. There can be multiple zones (instances) of the same map (instanced dungeons)
	// ---------------------------------------------------------------------------------------------------------------------------
	class Zone
	{
	private:
		const MapDef*	m_mapDef = nullptr; // Assigned in LoadZoneFromMap
		uint32_t		m_zoneID = 0;

		// Zone state
		bool m_isActive; // if the Zone is active or not. Inactive Zones will skip updating during a simulation step

		std::unordered_map<uint64_t, std::unique_ptr<Entity>>	m_entities;
		std::vector<Cell> m_cellMap;

		// To perform Entity Cell Transfer
		std::vector<EntityTransferCtx> m_entitiesWaitingForTransfer;

	private:
		// Entity Transfer
		void TransferPendingEntities();

	public:
		Zone(uint32_t mapID, uint32_t zoneID) : m_isActive(true), m_zoneID(zoneID)
		{
			// Initialize the m_cellMap in base of the loaded m_mapDef->m_mapID
			// TODO: for now maps that fail loading prevents the server to startup
			if (LoadZoneFromMap(mapID) != 0)
				throw std::runtime_error("LoadZoneFromMap failed!");
		}

		const MapDef* GetDef() const { return m_mapDef; }

		int		LoadZoneFromMap(uint32_t mapID);

		void	SetActive(bool v);
		void	Update(uint32_t diff);

		const uint32_t	GetZoneID() const	{ return m_zoneID; };

		const uint32_t	GetMapID() const	{ return m_mapDef->m_mapID; };
		const int		GetWidth() const	{ return m_mapDef->m_width; };
		const int		GetHeight() const	{ return m_mapDef->m_height; };
		const int		GetNLayers() const	{ return m_mapDef->m_nLayers; };

		// Entities Management
		Entity*		AddEntityToZone(std::unique_ptr<Entity> e);
		bool		RemoveEntityFromZone(uint64_t guid);
		void		AddPendingEntityToTransfer(EntityTransferCtx ctx);

		Entity*		FindEntity(uint64_t guid) const;
	};
}
}
