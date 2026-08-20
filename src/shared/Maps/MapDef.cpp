#include "MapDef.h"

#include "ConsoleLogger.h"
#include "FileLogger.h"

namespace NECRO
{
	MapDef::MapDef(uint32_t mapID, const NDB* mapDb)
	{
		try
		{
			m_mapID = mapID;

			// Load Fields (Hard Gets can throw here, but it's fine since if the definitions fail to load, we can't start the server)
			m_mapName = mapDb->GetString(m_mapID, "MapName");
			m_mapType = mapDb->GetInt(m_mapID, "MapType");

			m_width		= mapDb->GetInt(m_mapID, "Width");
			m_height	= mapDb->GetInt(m_mapID, "Height");
			m_nLayers	= mapDb->GetInt(m_mapID, "NLayers");

			m_mapFileName = mapDb->GetString(m_mapID, "MapFileName");

			// Load Map Static Definition

		}
		catch (const std::exception& e)
		{
			LOG_ERROR("Could not load MapDef! Exception: '{}'", e.what());
			throw std::runtime_error("Could not load MapDef! Aborting...");
		}
	}
}
