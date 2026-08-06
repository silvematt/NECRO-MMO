#pragma once

#include <cstdint>

namespace NECRO
{
namespace World
{
	class Cell;
	class Map;

	enum class EntityType
	{
		NULLTYPE = 0,
		PLAYER_ENTITY,
		AI_ENTITY
	};

	class Entity
	{
		friend Map;
	protected:
		uint64_t	m_guid;
		EntityType	m_type;
		bool		m_isActive;

		// Current spatial position
		Cell*		m_currentCell;
		Map*		m_currentMap;
		
		// Sets here are only for updating the back pointers, not for transferring entities between cells/maps
		void	SetCurrentMapPtr(Map* newMap);
		void	SetCurrentCellPtr(Cell* newCell);

	public:
		// Classes that inherit from this will need cleanup, entities will be destroyed from the Map::m_entities
		virtual		~Entity() = default;

		float		m_posX;
		float		m_posY;

		Entity(uint64_t guid, EntityType tpe) : m_guid(guid), m_type(tpe), m_currentMap(nullptr), m_currentCell(nullptr), m_isActive(true), m_posX(0), m_posY(0)
		{
		}

		Entity(uint64_t guid, EntityType tpe, float posX, float posY) : m_guid(guid), m_type(tpe), m_currentMap(nullptr), m_currentCell(nullptr),  m_isActive(true), m_posX(posX), m_posY(posY)
		{
		}

		const uint64_t GetGUID() const
		{
			return m_guid;
		}

		virtual void Update(uint32_t diff);
	};
}
}
