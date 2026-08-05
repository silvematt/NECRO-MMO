#include "Entity.h"
#include "Cell.h"

#include "ConsoleLogger.h"
#include "FileLogger.h"

namespace NECRO
{
namespace World
{
	void Entity::Update(uint32_t diff)
	{

	}

	void Entity::SetCurrentMapPtr(Map* m)
	{
		if (m)
		{
			m_currentMap = m;
		}
		else
		{
			m_currentMap = nullptr;
			LOG_ERROR("Tried to set Entity's with GUID '{}' new Map, but the passed ptr was null!", m_guid);

			// Entity is an invalid state, TODO decide what to do
		}
	}

	void Entity::SetCurrentCellPtr(Cell* c)
	{
		if (c)
		{
			m_currentCell = c;
		}
		else
		{
			m_currentCell = nullptr;
			LOG_ERROR("Tried to set Entity's with GUID '{}' new cell, but the passed ptr was null!", m_guid);

			// Entity is an invalid state, TODO decide what to do
		}
	}
}
}
