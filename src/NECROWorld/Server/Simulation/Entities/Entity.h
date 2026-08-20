#pragma once

#include <cstdint>
#include "GameRules.h"

namespace NECRO
{
namespace World
{
	class Cell;
	class Zone;

	enum class EntityType
	{
		NULLTYPE = 0,
		PLAYER_ENTITY,
		AI_ENTITY
	};

	class Entity;
	// This represents a request an entity made to be transferred into another cell. The server will validate it and, if considered valid, will transfer the entity
	struct EntityTransferCtx
	{
		uint64_t entityToTransferGUID;
		int		newGridPosX;
		int		newGridPosY;

		EntityTransferCtx(uint64_t e, int x, int y) : entityToTransferGUID(e), newGridPosX(x), newGridPosY(y) {};
	};

	class Entity
	{
		friend Zone;
	protected:
		uint64_t	m_guid;
		EntityType	m_type;
		bool		m_isActive;

		// Current spatial position
		Zone*		m_currentZone;
		Cell*		m_currentCell;

		// Grid Position - recalculated on every update
		int			m_curGridPosX = 0;
		int			m_curGridPosY = 0;
		int			m_curZLayer = 0;
		
		// Sets here are only for updating the back pointers, not for transferring entities between cells/zones
		void	SetCurrentZonePtr(Zone* newZone);
		void	SetCurrentCellPtr(Cell* newCell);

	public:
		// Classes that inherit from this will need cleanup, entities will be destroyed from the Zone::m_entities
		virtual		~Entity() = default;

		// Position
		float			m_posX;
		float			m_posY;
		float			m_posZ;
		IsoDirection	m_isoDirection;

		Entity(uint64_t guid, EntityType tpe) : m_guid(guid), m_type(tpe), m_currentZone(nullptr), m_currentCell(nullptr), m_isActive(true), m_posX(0), m_posY(0), m_posZ(0), m_isoDirection(IsoDirection::SOUTH)
		{
		}

		const uint64_t GetGUID() const
		{
			return m_guid;
		}

		virtual void Update(uint32_t diff);

		// Called as soon as the Entity is added to the map (via Zone::AddEntityToZone) and (m_currentZone, m_currentCell) are assigned
		virtual void OnBeingAddedToZone();

		// Called just before being removed from the map by (Zone::RemoveFromZone)
		virtual void OnBeingRemovedFromZone();

		// Called when the entity changes cell via SetCurrentCellPtr
		virtual void OnCellChanges();

		// Called when the Zone tried to cell-transfer this entity, but it failed in the 
		virtual void OnCellTransferFails();

		// TODO Called wehn the entity is trasferred (or being transferred?) to another Zone
		// virtual void OnBeingTransferredToNewZone(...)
	};
}
}
