#include "Cell.h"

#include "NECROClient.h"
#include "ClientUtility.h"
#include "NMath.h"

//--------------------------------------
// Constructor
//--------------------------------------
Cell::Cell()
{
	world = nullptr;
	cellX = cellY = 0;
	isoX = isoY = 0,
	dstRect = { 0,0,0,0 };
}

//--------------------------------------
// Sets the coordinates of this cells 
// relative to its world
//--------------------------------------
void Cell::SetCellCoordinates(const int x, const int y)
{
	cellX = x;
	cellY = y;

	NMath::CartToIso(cellX, cellY, isoX, isoY);

	// Adjust isoX so that the top of the image becomes the origin
	isoX -= HALF_CELL_WIDTH;

	// Adjust isoX and isoY to the world offset
	isoX += engine.GetGame().GetMainCamera()->pos.x;
	isoY += engine.GetGame().GetMainCamera()->pos.y;
	isoY -= dstRect.h; // bottom-left origin

	dstRect = { isoX, isoY, CELL_WIDTH, CELL_HEIGHT };
}

//--------------------------------------
// Sets the world pointer 
//--------------------------------------
void Cell::SetWorld(World* w)
{
	world = w;

	baseColor = w->GetBaseLightColor();
	baseIntensity = w->GetBaseLightIntensity();

	lColor = baseColor;
	lIntensity = baseIntensity;
}

//--------------------------------------
// Updates the Cell and its content
//--------------------------------------
void Cell::Update()
{
	NMath::CartToIso(cellX, cellY, isoX, isoY);

	// Adjust isoX so that the top of the image becomes the origin
	isoX -= HALF_CELL_WIDTH;

	// Adjust isoX and isoY to the world offset
	isoX += engine.GetGame().GetMainCamera()->pos.x;
	isoY += engine.GetGame().GetMainCamera()->pos.y;
	isoY -= dstRect.h; // bottom-left origin

	dstRect = { isoX, isoY, CELL_WIDTH, CELL_HEIGHT };

	for (auto& ent : entities)
		if (ent)
			ent->Update();
}

//-----------------------------------------------------------------------
// Adds an Entity to the entities ptrs vector
//-----------------------------------------------------------------------
void Cell::AddEntityPtr(Entity* e)
{
	e->SetOwner(this); // TODO: make sure adding the entityptr should set ownership, in the future one entity may occupy more than one cell
	entities.push_back(e);

	// If the entity modifies the cell Z modifier, apply it to the cell now
	float eMod = e->GetZCellModifier();
	if (eMod > 0 && eMod > zModifier)
		SetZModifier(eMod);
}

//--------------------------------------
// Removes an entity from the vector
//--------------------------------------
void Cell::RemoveEntityPtr(uint32_t remID)
{
	for (size_t i = 0; i < entities.size(); i++)
	{
		if (entities[i]->GetID() == remID)
		{
			RemoveEntityPtr(i);
			return;
		}
	}
}

//--------------------------------------
// Removes an entity from the vector
//--------------------------------------
void Cell::RemoveEntityPtr(size_t idx)
{
	entities.at(idx)->ClearOwner();
	ClientUtility::RemoveFromVector(entities, idx);
}


Entity* Cell::GetEntityPtr(uint32_t atID)
{
	for (size_t i = 0; i < entities.size(); i++)
	{
		if (entities[i]->GetID() == atID)
		{
			return GetEntityPtrAt(i);
		}
	}

	return nullptr;
}

Entity* Cell::GetEntityPtrAt(size_t indx)
{
	if (indx < entities.size())
		return entities.at(indx);

	return nullptr;
}

//--------------------------------------
// Draws all the entities of this cell
//--------------------------------------
void Cell::DrawEntities()
{
	for (auto& ent : entities)
		if (ent)
			ent->Draw();
}

//------------------------------------------------------------------
// Return if there is an entity in this Cell that blocks light
//------------------------------------------------------------------
bool Cell::BlocksLight()
{
	for (auto& ent : entities)
		if (ent && ent->BlocksLight()) // if an entity that blocks light sits in this cell, then this cell blocks light
			return true;

	return false;
}

//------------------------------------------------------------------
// Returns the amount of light blocked by this cell
//------------------------------------------------------------------
float Cell::GetLightBlockPercent()
{
	for (auto& ent : entities)
		if (ent && ent->BlocksLight()) // TODO: may accumulate light block value for all entites instead of just picking the first found
			return ent->GetLightBlockValue();

	return 0.0f;
}

//-----------------------------------------------------------------------------
// Given a light, computes how much illumination has to change in this cell
//-----------------------------------------------------------------------------
void Cell::SetLightingInfluence(Light* l, float dropoff, float occlusion)
{
	if (dropoff == 0)
		dropoff = 1;

	// If the light is dropping off by far, accentuate it so we don't create a barely lit square
	if (dropoff > l->farDropoffThreshold)
		dropoff *= l->farDropoffMultiplier;

	dropoff *= l->dropoffMultiplier + occlusion;

	SetLightingIntensity(lIntensity + (l->intensity / dropoff));
	SetLightingColor(lColor.r + ((l->color.r / dropoff) * l->intensity), 
					 lColor.g + ((l->color.g / dropoff) * l->intensity), 
					 lColor.b + ((l->color.b / dropoff) * l->intensity));
}

void Cell::AddEntitiesAsVisible()
{
	Camera* curCam = engine.GetGame().GetMainCamera();
	for (auto& ent : entities)
	{
		if (ent)
		{
			// Everything on the layer 0 does not need sorting unless it's dynamic
			if (ent->GetLayer() == 0 && !ent->TestFlag(Entity::Flags::Dynamic))
				curCam->AddToVisibleStaticEntities(ent);
			else
				curCam->AddToVisibleEntities(ent);
		}
	}
}
