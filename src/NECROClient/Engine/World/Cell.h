#ifndef NECROCELL_H
#define NECROCELL_H

#include <vector>
#include <memory>

#include "SDL.h"

#include "ClientUtility.h"
#include "Entity.h"

namespace NECRO
{
namespace Client
{
	const int CELL_WIDTH = 64;
	const int CELL_HEIGHT = 32;

	const int HALF_CELL_WIDTH = 32;
	const int HALF_CELL_HEIGHT = 16;

	// Forward Declaration
	class World;

	//-------------------------------------------------
	// A Cell represents a container for each tile in
	// the World. It has a base object (tile), collision
	// and entity information.
	// 
	// We can flag as composite Objects that will occupy 
	// more than one cell, and return all the cells that 
	// they occupy.
	//-------------------------------------------------
	class Cell
	{

	private:
		World* world; // refers to this cell's world

		int cellX, cellY;
		int isoX, isoY;

		SDL_Rect dstRect;				// dstRect used to DrawImageDirectly

		// Entity ptrs logically owned by this cell
		std::vector<Entity*> entities;

		// Lighting
		// Base
		SDL_Color baseColor;
		float baseIntensity;

		// Actual color and intensity, calculated every frame, used to render 
		SDL_Color lColor;
		float lIntensity;

		// ZModifier is used to modify the Z position of dynamic entities.
		// When an entity enters in a cell with a ZModifier != 0, his Z position (and therefore layer) becomes the one of the Z modifier
		// Used to allow entities to travel layers
		float zModifier = 0.0f;

	public:
		Cell();

		int				GetCellX() const;
		int				GetCellY() const;
		SDL_Rect& GetDstRect();
		World* GetWorld();

		SDL_Color* GetLightingColor();
		float& GetLightingIntensity();

		void			SetLightingIntensity(float i);
		void			SetLightingColor(int r, int g, int b);
		void			SetLightingInfluence(Light* l, float dropoff, float occlusion);

		void			SetWorld(World* w);
		void			SetCellCoordinates(const int x, const int y);

		void			Update();

		void			AddEntityPtr(Entity* e);
		void			RemoveEntityPtr(uint32_t remID);
		void			RemoveEntityPtr(size_t indx);
		size_t			GetEntitiesPtrSize() const;

		Entity* GetEntityPtr(uint32_t atID);
		Entity* GetEntityPtrAt(size_t indx);

		void			AddEntitiesAsVisible(); // adds all the entities in this cell to the visibleEntities list of the current camera, to sort and draw them at the end of the frame
		void			DrawEntities();

		bool			BlocksLight();
		float			GetLightBlockPercent();

		float			GetZModifier() const;
		void			SetZModifier(float v);
	};


	inline int Cell::GetCellX() const
	{
		return cellX;
	}

	inline int Cell::GetCellY() const
	{
		return cellY;
	}

	inline SDL_Rect& Cell::GetDstRect()
	{
		return dstRect;
	}

	inline World* Cell::GetWorld()
	{
		return world;
	}

	inline size_t Cell::GetEntitiesPtrSize() const
	{
		return entities.size();
	}

	inline SDL_Color* Cell::GetLightingColor()
	{
		return &lColor;
	}

	inline float& Cell::GetLightingIntensity()
	{
		return lIntensity;
	}

	inline void Cell::SetLightingColor(int r, int g, int b)
	{
		r = SDL_clamp(r, 0, 255);
		g = SDL_clamp(g, 0, 255);
		b = SDL_clamp(b, 0, 255);

		lColor.r = r;
		lColor.g = g;
		lColor.b = b;
	}

	inline void Cell::SetLightingIntensity(float i)
	{
		i = ClientUtility::Clampf(i, 0.0f, 1.0f);
		lIntensity = i;
	}

	inline float Cell::GetZModifier() const
	{
		return zModifier;
	}

	inline void Cell::SetZModifier(float v)
	{
		zModifier = v;
	}

}
}

#endif
