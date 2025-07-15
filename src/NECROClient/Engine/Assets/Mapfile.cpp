#include "Mapfile.h"
#include "NECROEngine.h"
#include "ClientUtility.h"
#include "TilesetDef.h"

#include "SDL.h"

#include <fstream>

namespace NECRO
{
namespace Client
{
	//----------------------------------------------------------------------------------------
	// Loads a Map as the current world. TODO: this overrides the previous world,
	// if in the we want to keep the exterior world after entering a dungeon this isn't ideal. 
	//----------------------------------------------------------------------------------------
	bool Mapfile::LoadMap(const std::string& filename)
	{
		// Get reference to the world
		World* w = engine.GetGame().GetCurrentWorld();

		std::string fullPath = MAPFILES_DIRECTORY + filename;

		std::fstream stream(fullPath);

		SDL_Log("Loading Mapfile at: '%s'\n", filename.c_str());

		if (!stream.is_open())
		{
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not load prefab at: %s\n", filename.c_str());
			return false;
		}

		std::string curLine;
		std::string curValStr;
		int curValInt;

		// World name
		std::getline(stream, curLine);
		curValStr = curLine.substr(curLine.find("=") + 2); // key = value;
		curValStr = curValStr.substr(0, curValStr.find(";"));
		m_name = curValStr;

		// tilesetDefName
		std::getline(stream, curLine);
		curValStr = curLine.substr(curLine.find("=") + 2); // key = value;
		curValStr = curValStr.substr(0, curValStr.find(";"));
		m_tilesetDefName = curValStr;

		// Load the tileset definition
		w->m_tileDef.LoadFromFile(m_tilesetDefName);

		// width
		std::getline(stream, curLine);
		curValStr = curLine.substr(curLine.find("=") + 2); // key = value;
		curValStr = curValStr.substr(0, curValStr.find(";"));
		m_width = ClientUtility::TryParseInt(curValStr);

		// height
		std::getline(stream, curLine);
		curValStr = curLine.substr(curLine.find("=") + 2); // key = value;
		curValStr = curValStr.substr(0, curValStr.find(";"));
		m_height = ClientUtility::TryParseInt(curValStr);

		// nLayers
		std::getline(stream, curLine);
		curValStr = curLine.substr(curLine.find("=") + 2); // key = value;
		curValStr = curValStr.substr(0, curValStr.find(";"));
		m_nLayers = ClientUtility::TryParseInt(curValStr);

		// Load layers and add tiles as entities
		bool layersDone = false;
		int curLayer = 0;
		std::getline(stream, curLine); // layerX:
		while (!layersDone)
		{
			std::getline(stream, curLine); // {

			// Load the matrix
			for (int i = 0; i < m_height; i++)
			{
				std::getline(stream, curLine); // get row
				// StartPos to read matrix values
				int startPos = 0;
				for (int j = 0; j < m_width; j++)
				{
					int endPos = curLine.find(',', startPos + 1);  // read until the ','

					// Get the TileID
					int curVal = ClientUtility::TryParseInt(curLine.substr(startPos, endPos - startPos));
					if (curVal != 0) // if not empty
					{
						// Get the resource img this value comes from
						int resIndex = w->m_tileDef.GetResourceIndexFromID(curVal);

						// Spawn the Tile as an Entity
						TilesetDef::TileData* data = w->m_tileDef.GetTileData(curVal);

						std::unique_ptr<Entity> e(new Entity(Vector2(static_cast<float>(CELL_WIDTH * j), static_cast<float>(CELL_HEIGHT * i)), w->m_tileDef.GetResource(resIndex)));
						e->SetLayer(curLayer);
						e->SetTilesetOffset(w->m_tileDef.GetTile(curVal).first, w->m_tileDef.GetTile(curVal).second);

						// If per-tile data is defined, apply it
						if (data)
						{
							e->m_zPos = data->zOffset;

							if (data->colliderEnabled)
							{
								e->CreateCollider();
								e->GetCollider()->Init(1, e.get(), data->collOffsetX, data->collOffsetY, data->collWidth, data->collHeight);
							}

							if (data->occlusionEnabled)
							{
								e->SetFlag(Entity::Flags::FCanOccludePlayer);
								e->SetOcclusionModifierValues(data->occlOffX, data->occlOffY);
							}

							e->SetZCellModifier(data->zCellModifier);
						}

						w->AddEntity(std::move(e));
					}

					startPos = endPos + 1; // adjust startPos for next reading
				}
			}

			std::getline(stream, curLine); // };

			std::getline(stream, curLine); // either a new layer, start of PrefabsList

			if (curLine.find("layer") != std::string::npos)
			{
				layersDone = false;
				curLayer++;
			}
			else
				layersDone = true;
		}

		// Load PrefabsList
		bool prefabsDone = false;

		// curLine is 'PrefabList'
		std::getline(stream, curLine); // {

		// Start of list

		while (!prefabsDone)
		{
			std::getline(stream, curLine);

			// Allow empty spaces between prefab definitions or comments
			while (ClientUtility::IsWhitespaceString(curLine) ||
				ClientUtility::IsCommentLine(curLine))
				std::getline(stream, curLine);

			// Check if it's the end
			if (curLine == "};")
				prefabsDone = true;
			else
			{
				int startPos = 0;
				int endPos = curLine.find(',', startPos + 1);  // read until the ','
				std::string prefabName = (curLine.substr(startPos, endPos - startPos));
				ClientUtility::RemoveSpacesAndTabsFromString(prefabName);

				startPos = endPos + 1;
				endPos = curLine.find(',', startPos + 1);
				float xPos = ClientUtility::TryParseFloat(curLine.substr(startPos, endPos - startPos));

				startPos = endPos + 1;
				endPos = curLine.find(',', startPos + 1);
				float yPos = ClientUtility::TryParseFloat(curLine.substr(startPos, endPos - startPos));

				startPos = endPos + 1;
				endPos = curLine.find(',', startPos + 1);
				float zPos = ClientUtility::TryParseFloat(curLine.substr(startPos, endPos - startPos));

				SDL_Log("Instantiating Prefab from List: '%s' | PARAMS: (%f, %f, %f)", prefabName.c_str(), xPos, yPos, zPos);

				std::unique_ptr<Entity> prefab = Prefab::InstantiatePrefab(prefabName, Vector2(xPos, yPos));

				if (prefab != NULL)
				{
					prefab->m_zPos = zPos;
					prefab->SetLayer(prefab->GetLayerFromZPos());

					w->AddEntity(std::move(prefab));
				}
			}
		}

		return true;
	}

}
}
