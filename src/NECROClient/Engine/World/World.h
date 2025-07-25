#ifndef NECROWORLD_H
#define NECROWORLD_H

#include <unordered_map>
#include <memory>

#include "Renderer.h"
#include "Cell.h"
#include "Mapfile.h"
#include "TilesetDef.h"

namespace NECRO
{
namespace Client
{
	inline constexpr int WORLD_WIDTH = 50; //TODO make this variable and specified in Mapfile
	inline constexpr int WORLD_HEIGHT = 50;

	// Constant added to visible min/max
	inline constexpr int VISIBLE_X_PLUS_OFFSET = 4;
	inline constexpr int VISIBLE_Y_PLUS_OFFSET = 4;

	//-------------------------------------------------
	// A World represents a level or a map with its
	// cells and entities that lives in them.
	// 
	// The World is composed of a 2D array of Cells.
	//-------------------------------------------------
	class World
	{
		friend Mapfile;

	private:
		Mapfile		m_map;
		TilesetDef	m_tileDef;

		Cell			m_worldmap[WORLD_WIDTH][WORLD_HEIGHT];
		Cell*			m_worldCursor = nullptr;					// The cell the mouse is currently hovering on (if any)
		SDL_Texture*	m_worldCursorEditTexture;
		SDL_Texture*	m_worldCursorPlayTexture;


		// Min/Max visible space to render and update
		int m_visibleMinX = 0;
		int m_visibleMaxX = 0;
		int m_visibleMinY = 0;
		int m_visibleMaxY = 0;

		//--------------------------------------------------------------------------------------
		// Entities physically exist and are phyiscally owned by the World they're spawned in.
		// Every entity exists inside the allEntities unordered_map, indexed by EntityID.
		// 
		// Entities are logically owned by Cells, where each Cell stores a vector of raw pointers
		// to the allEntities they logically own. That means we can quickly get any entity by ID
		// and given the Cell, we can quickly get all the entities contained in that cell.
		//--------------------------------------------------------------------------------------
		std::unordered_map<uint32_t, std::unique_ptr<Entity>> m_allEntities;

		// See Entity::TransferToCellImmediately
		std::vector<Entity*> m_entitiesWaitingForTransfer;

		// Lighting, for how lighting works it's better to have the world totally black and let entities with lights lit it, because if we apply a base color like gray (128,128,128), we can never have a fully red zone (255, 0, 0)
		SDL_Color	m_baseLightColor = colorWhite;
		float		m_baseLight = 1;

		// True if the player can interact with hovered entity during PLAY_MODE
		// Used to draw the cell highlight
		bool m_canInteract = false;

	private:
		void			UpdateVisibleCoords(); // updates visibleMinMax variables based on curCamera position and zoom

		void			ComputeOnSelectedCell(); // allows to perform actions on the selected cell, called in Update
		void			DrawUI();

	public:
		const std::unordered_map<uint32_t, std::unique_ptr<Entity>>& GetEntities();

		Cell* GetCellAt(int x, int y);
		float			GetBaseLightIntensity() const;
		SDL_Color		GetBaseLightColor() const;
		void			ResetLighting(); // Sets the current lColor and lIntensity of the cell to the base color, so light can be applied again the current frame


		void			AddEntity(std::unique_ptr<Entity>&& e);
		void			RemoveEntity(uint32_t atID);

		void			InitializeWorld();
		void			Update();

		bool			IsInWorldBounds(int x, int y);

		void			AddPendingEntityToTransfer(Entity* e);
		void			TransferPendingEntities();		// Transferring the entity is done AFTER a world update, otherwise if an entity is fast enough (or framerate is low enough) 
		// the entity can be udated multiple times if the update is done through iterating over the worldmap[][] instead of the allEntities map
		void			Draw();
	};

	inline const std::unordered_map<uint32_t, std::unique_ptr<Entity>>& World::GetEntities()
	{
		return m_allEntities;
	}

	inline float World::GetBaseLightIntensity() const
	{
		return m_baseLight;
	}

	inline SDL_Color World::GetBaseLightColor() const
	{
		return m_baseLightColor;
	}

}
}

#endif
