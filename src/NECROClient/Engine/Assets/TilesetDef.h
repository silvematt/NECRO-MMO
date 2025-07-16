#ifndef NECROTILEDEF_H
#define NECROTILEDEF_H

#include "Image.h"
#include "Logger.h"
#include "ConsoleLogger.h"
#include "FileLogger.h"

#include <unordered_map>
#include <vector>

namespace NECRO
{
namespace Client
{
	// TilesetDefs folder and extension
	inline constexpr const char* TILESET_DEFS_FOLDER = "Data/maps/tiledefs/";
	inline constexpr const char* TILESET_DEFS_EXTENSION = ".ntdef";

	//----------------------------------------------------------------------------------------------
	// A TilesetDef contains all the images of a tileset indexed by an int, used in the Mapfile.
	// It can have multiple tileset images loaded and indexed sequentially.
	//----------------------------------------------------------------------------------------------
	class TilesetDef
	{
	public:
		struct TileData
		{
			float	zOffset;
			bool	colliderEnabled;
			int		collOffsetX;
			int		collOffsetY;
			int		collWidth;
			int		collHeight;
			bool	occlusionEnabled;
			int		occlOffX;
			int		occlOffY;
			float	zCellModifier;

			TileData(float zOff, bool cEnabled, int cOffX, int cOffY, int cWidth, int cHeight, bool oEnabled, int oXOff, int oYOff, float zMod) :
				zOffset(zOff),
				colliderEnabled(cEnabled),
				collOffsetX(cOffX),
				collOffsetY(cOffY),
				collWidth(cWidth),
				collHeight(cHeight),
				occlusionEnabled(oEnabled),
				occlOffX(oXOff),
				occlOffY(oYOff),
				zCellModifier(zMod)
			{
			}
		};

	private:
		bool m_loaded = false;

		std::string m_name;
		int			m_nTilesets; // the number of tileset images in this definition

		std::vector<Image*>					m_resources;			// The imgs ptrs to the actual image loaded in the assets manager
		std::vector<std::pair<int, int>>	m_resourceEndMap;		// Maps the Resource ID and the last tile ID of that resource (the actual .png image), so we can know translate by tileID what resource contains that tile
		std::vector<std::pair<int, int>>	m_tiles;				// All tiles of this TilesetDef, #row, #col of the corresponding tileset image (index of vector) - es tiles(145).first | tiles(145).second returns the X,Y offsets to the tile with ID 145 
		std::unordered_map<int, TileData>	m_tilesData;			// Per - tile data definition, like collision


	public:
		bool		LoadFromFile(const std::string& filename);
		bool		IsLoaded();

		Image*				GetResource(int indx);
		std::pair<int, int>	GetResourceEndMap(int indx);
		std::pair<int, int> GetTile(int indx);
		TileData*			GetTileData(int ID);

		int					GetResourceIndexFromID(int ID);
	};

	inline bool TilesetDef::IsLoaded()
	{
		return m_loaded;
	}

	inline Image* TilesetDef::GetResource(int indx)
	{
		if (indx < m_resources.size())
			return m_resources[indx];

		return nullptr;
	}

	inline std::pair<int, int> TilesetDef::GetResourceEndMap(int indx)
	{
		if (indx < m_tiles.size())
			return m_tiles[indx];

		LOG_WARNING("Warning! Accessing TileDef '{}' resources_end_map was out of bound!", m_name.c_str());

		return std::make_pair<int, int>(0, 0);
	}

	inline std::pair<int, int> TilesetDef::GetTile(int indx)
	{
		if (indx < m_tiles.size())
			return m_tiles[indx];

		LOG_WARNING("Warning! Accessing TileDef '{}' resources_end_map was out of bound!", m_name.c_str());

		return std::make_pair<int, int>(0, 0);
	}

	//----------------------------------------------------------------------------------------------
	// Given the ID of a tile, returns the ID of the resource the tile comes from
	//----------------------------------------------------------------------------------------------
	inline int TilesetDef::GetResourceIndexFromID(int ID)
	{
		if (!m_loaded)
		{
			LOG_WARNING("Warning! Called GetResourceIndexFromID on TileDef: '{}' but it was not loaded!", m_name.c_str());
			return -1;
		}

		for (int i = 0; i < m_nTilesets; i++)
		{
			if (ID <= m_resourceEndMap.at(i).second) // if the ID is less or equal the last element of the current iterating resource
			{
				return i;
			}
		}

		return 0;
	}

	inline TilesetDef::TileData* TilesetDef::GetTileData(int ID)
	{
		auto it = m_tilesData.find(ID);
		if (it != m_tilesData.end())
		{
			return &it->second;
		}

		return nullptr;
	}

}
}

#endif
