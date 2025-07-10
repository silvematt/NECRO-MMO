#ifndef NECROMAPFILE_H
#define NECROMAPFILE_H

#include <string>

#define MAPFILES_DIRECTORY "Data/maps/"

//------------------------------------------------------------------------------
// Contains the Map definition that specifies the World structure and content.
//------------------------------------------------------------------------------
class Mapfile
{
public:
	std::string name;
	std::string tilesetDefName;	// A Mapfile uses a TilesetDef to specify the tiles used in the layers matrices
	int width;
	int height;
	int nLayers; // number of layers of this map

	bool			LoadMap(const std::string& filename);
};

#endif
