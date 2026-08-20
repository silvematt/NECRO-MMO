#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <memory>
#include <unordered_map>

#include "MapDef.h"
#include "Cell.h"
#include "Entity.h"

namespace NECRO
{
namespace World
{
	// ------------------------------------------------------------------------------------------------------------------------
	// A loaded map in the Server, also called "Zone". It can be an exterior, a dungeon or anything in between.
	// A Zone is an entry in the std::unordered_map<uint32_t, std::unique_ptr<Map>>	m_zones and it represents a loaded Map.
	// 
	// Will always be child of the WorldSimulation. There can be multiple instances of the same map (instanced dungeons)
	// ------------------------------------------------------------------------------------------------------------------------
	class Map
	{
	private:
		MapDef	m_mapDef;

		// Map state
		bool m_isActive; // if the map is active or not. Inactive maps will skip updating during a simulation step of

		std::unordered_map<uint64_t, std::unique_ptr<Entity>>	m_entities;
		std::vector<Cell> m_cellMap;

		// To perform Entity Cell Transfer
		std::vector<EntityTransferCtx> m_entitiesWaitingForTransfer;

	private:
		// Entity Transfer
		void TransferPendingEntities();

	public:
		Map(uint32_t mapID) : m_isActive(true)
		{
			// Initialize the m_cellMap in base of the loaded m_mapID
			// TODO: for now maps that fail loading prevents the server to startup
			LoadMap(mapID);
		}

		const MapDef& GetDef() const { return m_mapDef; }

		int		LoadMap(uint32_t mapID);
		void	SetActive(bool v);
		void	Update(uint32_t diff);

		const uint32_t	GetMapID() const	{ return m_mapDef.m_mapID; };
		const int		GetWidth() const	{ return m_mapDef.m_width; };
		const int		GetHeight() const	{ return m_mapDef.m_height; };
		const int		GetNLayers() const	{ return m_mapDef.m_nLayers; };

		// Entities Management
		Entity*		AddEntityToMap(std::unique_ptr<Entity> e);
		bool		RemoveEntityFromMap(uint64_t guid);
		void		AddPendingEntityToTransfer(EntityTransferCtx ctx);

		Entity*		FindEntity(uint64_t guid) const;
	};
}
}
