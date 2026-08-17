#include "Entity.h"
#include "Cell.h"
#include "Map.h"

#include "ConsoleLogger.h"
#include "FileLogger.h"
#include "GameRules.h"

namespace NECRO
{
namespace World
{
	void Entity::Update(uint32_t diff)
	{
		int oldGridPosX = m_curGridPosX;
		int oldGridPosY = m_curGridPosY;
		int oldLayerZ	= m_curZLayer;

		// Update grid and layer pos
		int newGridPosX	= (m_posX / CELL_WIDTH);
		int newGridPosY	= (m_posY / CELL_HEIGHT);
		int newZLayer		= std::floor((m_posZ / LAYER_Z_COEFFICIENT));

		// For now trust the zLayer, but TODO, we should do a check against the map as well
		m_curZLayer = newZLayer;

		// Check if GridPos changed, if so, we need to transfer this Entity to the other Cell - but we need confirmation from the map.
		// The map will perform the transfer and if successfull, will update m_curGridPosX, m_curGridPosY and m_curZLayer.
		// If somehow the map doesn't accept the transfer (illegal pos, or the Cell is already full of entities), the entity will be snapped back to the center of the Cell(m_curGridPosX,m_curGridPosY)
		if (oldGridPosX != newGridPosX || oldGridPosY != newGridPosY)
		{
			m_currentMap->AddPendingEntityToTransfer(EntityTransferCtx(m_guid, newGridPosX, newGridPosY));
		}
		else
		{
			// No cell transfer, this is legit in-cell movement
			m_curGridPosX = newGridPosX;
			m_curGridPosY = newGridPosY;
		}
	}

	void Entity::SetCurrentMapPtr(Map* m)
	{
		if (m)
		{
			m_currentMap = m;
		}
		else
		{
			m_currentMap = nullptr;
			LOG_ERROR("Tried to set Entity's with GUID '{}' new Map, but the passed ptr was null!", m_guid);

			// Entity is an invalid state, TODO decide what to do
		}
	}

	void Entity::SetCurrentCellPtr(Cell* c)
	{
		if (c)
		{
			Cell* previousCell = m_currentCell;

			m_currentCell = c;
			m_curGridPosX = c->GetCellX();
			m_curGridPosY = c->GetCellY();
		}
		else
		{
			m_currentCell = nullptr;
			LOG_ERROR("Tried to set Entity's with GUID '{}' new cell, but the passed ptr was null!", m_guid);

			// Entity is an invalid state, TODO decide what to do
		}
	}

	// Called as soon as the Entity is added to the map (via Map::AddEntityToMap) and (m_currentMap, m_currentCell) are assigned
	void Entity::OnBeingAddedToMap()
	{

	}

	// Called just before being removed from the map by (Map::RemoveFromMap)
	void Entity::OnBeingRemovedFromMap()
	{

	}

	// Called from SetCurrentCellPtr after setting the new cell
	void Entity::OnCellChanges()
	{

	}

	// Called by the Map object if it fails to transfer this entity after he requested the transfer
	void Entity::OnCellTransferFails()
	{

	}
}
}
