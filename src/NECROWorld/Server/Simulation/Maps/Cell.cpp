#include "Cell.h"
#include <Entity.h>

#include "ConsoleLogger.h"
#include "FileLogger.h"

namespace NECRO
{
namespace World
{
	void Cell::Update(uint32_t diff)
	{
		for (int i = 0; i < m_entitiesHere.size(); i++)
		{
			m_entitiesHere[i]->Update(diff);
		}
	}

	bool Cell::AddEntityHere(Entity* e)
	{
		LOG_DEBUG("Entity {} added to cell ({},{})", e->GetGUID(), m_cellX, m_cellY);
		m_entitiesHere.push_back(e);
		return true;
	}

	bool Cell::RemoveEntityHere(uint64_t guid)
	{
		for (int i = 0; i < m_entitiesHere.size(); i++)
		{
			if (m_entitiesHere[i]->GetGUID() == guid)
			{
				m_entitiesHere.erase(m_entitiesHere.begin() + i);
				return true;
			}
		}

		return false;
	}
}
}
