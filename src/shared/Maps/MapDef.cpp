#include "MapDef.h"

#include "ConsoleLogger.h"
#include "FileLogger.h"

namespace NECRO
{
	bool MapDef::LoadMapDefinition(uint32_t mapID, const NDB* mapDb)
	{
		try
		{
			m_mapID = mapID;

			// Load Fields
			m_mapName = *mapDb->TryFind(m_mapID, "MapName")->AsString();
			m_mapType = *mapDb->TryFind(m_mapID, "MapType")->AsInt();

			m_width		= *mapDb->TryFind(m_mapID, "Width")->AsInt();
			m_height	= *mapDb->TryFind(m_mapID, "Height")->AsInt();
			m_nLayers	= *mapDb->TryFind(m_mapID, "NLayers")->AsInt();

			m_mapFileName = *mapDb->TryFind(m_mapID, "MapFileName")->AsString();

			// Load Map Static Definition

			return true;
		}
		catch (...)
		{
			LOG_ERROR("Could not load MapDef!");
			return false;
		}
	}
}
