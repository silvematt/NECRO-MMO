#ifndef NECROMAPFILE_H
#define NECROMAPFILE_H

#include <string>

namespace NECRO
{
namespace Client
{
	inline constexpr const char* MAPFILES_DIRECTORY = "Data/maps/";

	//------------------------------------------------------------------------------
	// Contains the Map definition that specifies the World structure and content.
	//------------------------------------------------------------------------------
	class Mapfile
	{
	public:
		std::string		m_name;
		std::string		m_tilesetDefName;	// A Mapfile uses a TilesetDef to specify the tiles used in the layers matrices

		int m_width;
		int m_height;
		int m_nLayers; // number of layers of this map

		bool			LoadMap(const std::string& filename);
	};

}
}

#endif
