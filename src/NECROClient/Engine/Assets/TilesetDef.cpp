#include "TilesetDef.h"
#include "ClientUtility.h"
#include "NECROEngine.h"
#include "Image.h"

#include "SDL.h"

#include <fstream>

namespace NECRO
{
namespace Client
{
	//----------------------------------------------------------------------------------------------
	// Load TilesetDef from a file.
	//----------------------------------------------------------------------------------------------
	bool TilesetDef::LoadFromFile(const std::string& filename)
	{
		std::string fullPath = TILESET_DEFS_FOLDER + filename;

		std::ifstream stream(fullPath);

		SDL_Log("Loading TilesetDef: '%s'\n", fullPath.c_str());

		if (!stream.is_open())
		{
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not load TilesetDef at: %s\n", fullPath.c_str());
			return false;
		}

		// Load base data
		std::string curLine;
		std::string curValStr;

		// TilesetDef name
		std::getline(stream, curLine);
		curValStr = curLine.substr(curLine.find("=") + 2); // key = value;
		curValStr = curValStr.substr(0, curValStr.find(";"));
		m_name = curValStr;

		m_nTilesets = 0; // filled as we read

		// Read actual images
		m_tiles.push_back(std::make_pair(0, 0)); // index 0 means no image, so add empty space

		bool doneReadingTilesets = false;
		int curID = 1; // id of tiles, starts at 1 because ID 0 means empty
		int curTilesetIndx = 0;
		while (!doneReadingTilesets)
		{
			std::getline(stream, curLine);

			if (curLine.find("ENDTILESETS") != std::string::npos)
			{
				doneReadingTilesets = true;
			}
			else
			{
				// Load Tileset until END is met
				while (1)
				{
					if (curLine == "END")
					{
						m_nTilesets++;
						curTilesetIndx++;
						break;
					}

					// If it's first line:
					if (curLine.find("src") != std::string::npos)
					{
						// Load the image for the first time
						curValStr = curLine.substr(curLine.find("=") + 2); // key = value;
						curValStr = curValStr.substr(0, curValStr.find(";"));

						SDL_Log("Loading Tileset %s in TilesetDef...\n", curValStr.c_str());

						// Get the actual Tileset image and load it
						Image* img = engine.GetAssetsManager().GetImage(curValStr);

						// Put image pointer in resources, so we can quickly access it with an int index
						m_resources.push_back(img);

						// Load the tiles
						if (img->IsTileset())
						{
							Image::Tileset* t = img->GetTileset();
							for (int y = 0; y < t->tileYNum; y++)
								for (int x = 0; x < t->tileXNum; x++)
								{
									m_tiles.push_back(std::make_pair(x, y));
									curID++;
								}
						}
						else
							LOG_WARNING("While loading the Tiledef '" + filename + "', a tileset named '" + curValStr.c_str() + "' was to be loaded, but IT IS NOT defined as a tileset in its definition file (or the definition file is missing). Skipping it.\nThis will cause a Resource ID mismatch.");

						// Keep end-map updated
						m_resourceEndMap.push_back(std::make_pair(curTilesetIndx, curID - 1));
					}
					else // Otherwise, it's tile-specific data
					{
						if (curLine.at(0) == '#') // skip comment
							std::getline(stream, curLine);

						// # curID, #zPos, #collEnabled, #collOffsetX, #collOffsetY, #collWidth, #collHeight
						// Read curID
						int startPos = 0;
						int endPos = curLine.find(',', startPos + 1);  // read until the ','

						int curID = ClientUtility::TryParseInt(curLine.substr(startPos, endPos - startPos));

						startPos = endPos + 1;
						endPos = curLine.find(',', startPos + 1);
						float zOffset = ClientUtility::TryParseInt(curLine.substr(startPos, endPos - startPos));

						startPos = endPos + 1;
						endPos = curLine.find(',', startPos + 1);
						bool cEnabled = ClientUtility::TryParseInt(curLine.substr(startPos, endPos - startPos));

						startPos = endPos + 1;
						endPos = curLine.find(',', startPos + 1);
						int collOffX = ClientUtility::TryParseInt(curLine.substr(startPos, endPos - startPos));

						startPos = endPos + 1;
						endPos = curLine.find(',', startPos + 1);
						int collOffY = ClientUtility::TryParseInt(curLine.substr(startPos, endPos - startPos));

						startPos = endPos + 1;
						endPos = curLine.find(',', startPos + 1);
						int collWidth = ClientUtility::TryParseInt(curLine.substr(startPos, endPos - startPos));

						startPos = endPos + 1;
						endPos = curLine.find(',', startPos + 1);
						int collHeight = ClientUtility::TryParseInt(curLine.substr(startPos, endPos - startPos));

						startPos = endPos + 1;
						endPos = curLine.find(',', startPos + 1);
						int occlEnabled = ClientUtility::TryParseInt(curLine.substr(startPos, endPos - startPos));

						startPos = endPos + 1;
						endPos = curLine.find(',', startPos + 1);
						int occlOffsetX = ClientUtility::TryParseInt(curLine.substr(startPos, endPos - startPos));

						startPos = endPos + 1;
						endPos = curLine.find(',', startPos + 1);
						int occlOffsetY = ClientUtility::TryParseInt(curLine.substr(startPos, endPos - startPos));

						startPos = endPos + 1;
						endPos = curLine.find(',', startPos + 1);
						float zMod = ClientUtility::TryParseFloat(curLine.substr(startPos, endPos - startPos));

						//SDL_Log("%d -> %f\n", curID, zOffset);

						m_tilesData.insert({ curID, TileData(zOffset, cEnabled, collOffX, collOffY, collWidth, collHeight, occlEnabled, occlOffsetX, occlOffsetY, zMod) });
					}

					std::getline(stream, curLine); // get next line
				}
			}
		}

		m_loaded = true;
	}

}
}
