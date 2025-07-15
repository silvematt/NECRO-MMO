#include "Cell.h"

#include "NECROEngine.h"
#include "ClientUtility.h"
#include "NMath.h"

namespace NECRO
{
namespace Client
{
	//--------------------------------------
	// Constructor
	//--------------------------------------
	Cell::Cell()
	{
		m_world = nullptr;
		m_cellX = m_cellY = 0;
		m_isoX = m_isoY = 0,
		m_dstRect = { 0,0,0,0 };
	}

	//--------------------------------------
	// Sets the coordinates of this cells 
	// relative to its world
	//--------------------------------------
	void Cell::SetCellCoordinates(const int x, const int y)
	{
		m_cellX = x;
		m_cellY = y;

		NMath::CartToIso(m_cellX, m_cellY, m_isoX, m_isoY);

		// Adjust isoX so that the top of the image becomes the origin
		m_isoX -= HALF_CELL_WIDTH;

		// Adjust isoX and isoY to the world offset
		m_isoX += engine.GetGame().GetMainCamera()->m_pos.x;
		m_isoY += engine.GetGame().GetMainCamera()->m_pos.y;
		m_isoY -= m_dstRect.h; // bottom-left origin

		m_dstRect = { m_isoX, m_isoY, CELL_WIDTH, CELL_HEIGHT };
	}

	//--------------------------------------
	// Sets the world pointer 
	//--------------------------------------
	void Cell::SetWorld(World* w)
	{
		m_world = w;

		m_baseColor = w->GetBaseLightColor();
		m_baseIntensity = w->GetBaseLightIntensity();

		m_lColor = m_baseColor;
		m_lIntensity = m_baseIntensity;
	}

	//--------------------------------------
	// Updates the Cell and its content
	//--------------------------------------
	void Cell::Update()
	{
		NMath::CartToIso(m_cellX, m_cellY, m_isoX, m_isoY);

		// Adjust isoX so that the top of the image becomes the origin
		m_isoX -= HALF_CELL_WIDTH;

		// Adjust isoX and isoY to the world offset
		m_isoX += engine.GetGame().GetMainCamera()->m_pos.x;
		m_isoY += engine.GetGame().GetMainCamera()->m_pos.y;
		m_isoY -= m_dstRect.h; // bottom-left origin

		m_dstRect = { m_isoX, m_isoY, CELL_WIDTH, CELL_HEIGHT };

		for (auto& ent : m_entities)
			if (ent)
				ent->Update();
	}

	//-----------------------------------------------------------------------
	// Adds an Entity to the entities ptrs vector
	//-----------------------------------------------------------------------
	void Cell::AddEntityPtr(Entity* e)
	{
		e->SetOwner(this); // TODO: make sure adding the entityptr should set ownership, in the future one entity may occupy more than one cell
		m_entities.push_back(e);

		// If the entity modifies the cell Z modifier, apply it to the cell now
		float eMod = e->GetZCellModifier();
		if (eMod > 0 && eMod > m_zModifier)
			SetZModifier(eMod);
	}

	//--------------------------------------
	// Removes an entity from the vector
	//--------------------------------------
	void Cell::RemoveEntityPtr(uint32_t remID)
	{
		for (size_t i = 0; i < m_entities.size(); i++)
		{
			if (m_entities[i]->GetID() == remID)
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
		m_entities.at(idx)->ClearOwner();
		ClientUtility::RemoveFromVector(m_entities, idx);
	}


	Entity* Cell::GetEntityPtr(uint32_t atID)
	{
		for (size_t i = 0; i < m_entities.size(); i++)
		{
			if (m_entities[i]->GetID() == atID)
			{
				return GetEntityPtrAt(i);
			}
		}

		return nullptr;
	}

	Entity* Cell::GetEntityPtrAt(size_t indx)
	{
		if (indx < m_entities.size())
			return m_entities.at(indx);

		return nullptr;
	}

	//--------------------------------------
	// Draws all the entities of this cell
	//--------------------------------------
	void Cell::DrawEntities()
	{
		for (auto& ent : m_entities)
			if (ent)
				ent->Draw();
	}

	//------------------------------------------------------------------
	// Return if there is an entity in this Cell that blocks light
	//------------------------------------------------------------------
	bool Cell::BlocksLight()
	{
		for (auto& ent : m_entities)
			if (ent && ent->BlocksLight()) // if an entity that blocks light sits in this cell, then this cell blocks light
				return true;

		return false;
	}

	//------------------------------------------------------------------
	// Returns the amount of light blocked by this cell
	//------------------------------------------------------------------
	float Cell::GetLightBlockPercent()
	{
		for (auto& ent : m_entities)
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
		if (dropoff > l->m_farDropoffThreshold)
			dropoff *= l->m_farDropoffMultiplier;

		dropoff *= l->m_dropoffMultiplier + occlusion;

		SetLightingIntensity(m_lIntensity + (l->m_intensity / dropoff));
		SetLightingColor(m_lColor.r + ((l->m_color.r / dropoff) * l->m_intensity),
			m_lColor.g + ((l->m_color.g / dropoff) * l->m_intensity),
			m_lColor.b + ((l->m_color.b / dropoff) * l->m_intensity));
	}

	void Cell::AddEntitiesAsVisible()
	{
		Camera* curCam = engine.GetGame().GetMainCamera();
		for (auto& ent : m_entities)
		{
			if (ent)
			{
				// Everything on the layer 0 does not need sorting unless it's dynamic
				if (ent->GetLayer() == 0 && !ent->TestFlag(Entity::Flags::FDynamic))
					curCam->AddToVisibleStaticEntities(ent);
				else
					curCam->AddToVisibleEntities(ent);
			}
		}
	}

}
}
