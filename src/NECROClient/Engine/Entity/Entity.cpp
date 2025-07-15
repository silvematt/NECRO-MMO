#include "NECROEngine.h"

#include "Entity.h"
#include "ClientUtility.h"
#include "Player.h"

namespace NECRO
{
namespace Client
{
	uint32_t Entity::ENT_NEXT_ID = 0;
	bool	Entity::DEBUG_COLLIDER_ENABLED = false;
	int		Entity::DEBUG_COLLIDER_LAYER = -1;
	bool	Entity::DEBUG_OCCLUSION_ENABLED = false;

	Entity::~Entity()
	{

	}

	Entity::Entity()
	{
		m_ID = ENT_NEXT_ID;
		ENT_NEXT_ID++;

		m_img = nullptr;
		m_owner = m_nextOwner = nullptr;
		m_tilesetXOff = m_tilesetYOff = 0;
		m_gridPosX = m_gridPosY = 0;
		m_occlusionRect = { 0,0,0,0 };
		m_toRender = true;
	}

	Entity::Entity(Vector2 pInitialPos, Image* pImg)
	{
		m_ID = ENT_NEXT_ID;
		ENT_NEXT_ID++;

		m_pos = pInitialPos;
		m_gridPosX = m_pos.x / CELL_WIDTH;
		m_gridPosY = m_pos.y / CELL_HEIGHT;
		m_zPos = 0;
		m_toRender = true;

		SetImg(pImg);
	}

	//------------------------------------------------------------
	// Sets the Image of this Entity
	//------------------------------------------------------------
	void Entity::SetImg(Image* pImg)
	{
		m_img = pImg;
	}

	//------------------------------------------------------------
	// Sets the owner of this Entity
	//------------------------------------------------------------
	void Entity::SetOwner(Cell* c)
	{
		m_owner = c;
	}

	//------------------------------------------------------------
	// Clears the owner of this entity
	//------------------------------------------------------------
	void Entity::ClearOwner()
	{
		m_owner = nullptr;
	}

	//-----------------------------------------------------------------------------------------------
	// Moves the entity to the cell specified in the argument, (if nullptr is passed, it's nextOwner)
	// removing the Ptr present in the previous owner's vector
	// 
	// Immediately: does the transfer right away, (as for now, as soon as Entity::Update is called)
	// If the entities are updated from looping over the worldmap, THIS MUST NOT BE USED, 
	// if an entity moves fast enough downwards (or framerate is low), the entity can be updated 
	// multiple times (from cell [0][0] to [4][4], and re-updated from [4][4] to [8][8], and so on)
	// 
	// Use TransferToCellQueue to let the world perform the transfer AFTER a world update completes.
	// This function will be called by the World with nullptr as argument
	//-----------------------------------------------------------------------------------------------
	void Entity::TransferToCellImmediately(Cell* c)
	{
		// If this is called with nullptr as argument, the transfer is supposed to be in nextOwner
		if (c == nullptr && m_nextOwner)
			c = m_nextOwner;

		// Remove Ptr from previous owner
		m_owner->RemoveEntityPtr(m_ID);

		// Update owner
		SetOwner(c);

		// Add Ptr to current owner
		c->AddEntityPtr(this);

		m_nextOwner = nullptr;

		OnCellChanges();
	}

	//-----------------------------------------------------------------------------------------------
	// TransferToCellQueue lets the world perform the transfer AFTER a world update completes.
	//-----------------------------------------------------------------------------------------------
	void Entity::TransferToCellQueue(Cell* c)
	{
		m_nextOwner = c;
		m_owner->GetWorld()->AddPendingEntityToTransfer(this);
	}

	//------------------------------------------------------------
	// Updates the Entity
	//------------------------------------------------------------
	void Entity::Update()
	{
		int oldGridPosX = m_gridPosX;
		int oldGridPosY = m_gridPosY;

		// Update grid position
		m_gridPosX = m_pos.x / CELL_WIDTH;
		m_gridPosY = m_pos.y / CELL_HEIGHT;

		// Update ISO coordinates
		NMath::CartToIso(m_pos.x / CELL_WIDTH, m_pos.y / CELL_HEIGHT, m_isoPos.x, m_isoPos.y);

		m_isoPos.x -= HALF_CELL_WIDTH;

		// Adjust isoX and isoY to the camera offset
		m_isoPos.x += engine.GetGame().GetMainCamera()->m_pos.x;
		m_isoPos.y += engine.GetGame().GetMainCamera()->m_pos.y;

		// Account for bottom-left origin
		if (!m_img->IsTileset())
			m_isoPos.y -= m_img->GetHeight();
		else
			m_isoPos.y -= m_img->GetTilesetHeight();

		// Account for the offset of the image
		m_isoPos.x += m_img->GetXOffset();
		m_isoPos.y += m_img->GetYOffset();

		// TODO: Naive solution? we can do topological sort instead of a single depth value if we encounter issues with this one for our goals, but it seems it can work
		m_depth = m_owner->GetCellX() + m_owner->GetCellY() + (LAYER_Z_COEFFICIENT * m_layer) + m_zPos; // zPos is used to position dynamic entities, like the player, or to offset static sprites.

		// If this entity has a collider and it is enabled, update it
		if (m_coll && m_coll->m_enabled)
			m_coll->Update();

		// Check for occlusion against player
		Entity* p = (Entity*)engine.GetGame().GetCurPlayer();
		if (p && p != this && TestFlag(Entity::Flags::FCanOccludePlayer))
		{
			// Check if this entity is close enough to the player to be worth testing intersection (first with gridPos, then with pos)
			if (abs(p->m_gridPosX - m_gridPosX) < ENTITY_OCCLUSION_TEST_X_DIFF && abs(p->m_gridPosY - m_gridPosY) < ENTITY_OCCLUSION_TEST_Y_DIFF &&
				(p->m_pos.x < this->m_pos.x || p->m_pos.y < this->m_pos.y) && SDL_HasIntersection(&m_occlusionRect, &p->m_occlusionRect))
				SetOccludes(true);
			else
				SetOccludes(false);
		}

		// If this Entity has a light, update it
		if (HasLight())
			GetLight()->Update();

		if (HasAnimator())
			GetAnimator()->Update();

		// Perform Cell trasfer if needed
		if (oldGridPosX != m_gridPosX || oldGridPosY != m_gridPosY)
		{
			m_nextOwner = m_owner->GetWorld()->GetCellAt(m_gridPosX, m_gridPosY);
			TransferToCellQueue(m_nextOwner); // will be done as soon as the world update is finished
		}
	}

	//------------------------------------------------------------
	// Looks at the lighting model and updates lightingColor to 
	// draw this entity
	//------------------------------------------------------------
	void Entity::UpdateLighting()
	{
		SDL_Color* cellColor = m_owner->GetLightingColor();
		m_lightingColor.r = cellColor->r * m_owner->GetLightingIntensity();
		m_lightingColor.g = cellColor->g * m_owner->GetLightingIntensity();
		m_lightingColor.b = cellColor->b * m_owner->GetLightingIntensity();
	}

	//------------------------------------------------------------
	// Draws the Entity
	//------------------------------------------------------------
	void Entity::Draw()
	{
		if (m_toRender)
		{
			UpdateLighting();

			// Save texture's alpha
			Uint8 previousAlpha = 0;
			SDL_GetTextureAlphaMod(m_img->GetSrc(), &previousAlpha);

			// Save texture's color
			Uint8 previousR, previousG, previousB;
			SDL_GetTextureColorMod(m_img->GetSrc(), &previousR, &previousG, &previousB);

			// Update alpha
			if (m_occludes)
				SDL_SetTextureAlphaMod(m_img->GetSrc(), OCCLUDED_SPRITE_ALPHA_VALUE);

			// Update Color with color data
			SDL_SetTextureColorMod(m_img->GetSrc(), m_lightingColor.r, m_lightingColor.g, m_lightingColor.b);

			if (!m_img->IsTileset())
			{
				SDL_Rect dstRect = { static_cast<int>(m_isoPos.x), static_cast<int>(m_isoPos.y), m_img->GetWidth(), m_img->GetHeight() };
				m_occlusionRect = dstRect;

				m_occlusionRect.w -= m_occlModifierX;
				m_occlusionRect.h -= m_occlModifierY;
				m_occlusionRect.x += (m_occlModifierX / 2);
				m_occlusionRect.y += (m_occlModifierY / 2);

				// Draw the occlusion debug on the Debug Render Target
				if (DEBUG_OCCLUSION_ENABLED && (TestFlag(FCanOccludePlayer) || m_ID == Player::ENT_ID))
				{
					// Set debug target
					auto previousTarget = engine.GetRenderer().GetCurrentERenderTargetVal();
					engine.GetRenderer().SetRenderTarget(Renderer::ETargets::DEBUG_TARGET);

					// Adjust for zoom
					float zoomLevel = engine.GetGame().GetMainCamera()->GetZoom();
					engine.GetRenderer().SetScale(zoomLevel, zoomLevel); // TODO: this should not be here (probably in SetZoom with the main RenderTarget scale), we need to set the scale of the renderer one time and not for each debug draw

					// Draw
					engine.GetRenderer().DrawRect(&m_occlusionRect, colorRed);

					// Restore previous target
					engine.GetRenderer().SetRenderTarget(previousTarget);
				}

				engine.GetRenderer().DrawImageDirectly(m_img->GetSrc(), NULL, &dstRect);
			}
			else
			{
				Image::Tileset* tset = m_img->GetTileset();
				SDL_Rect srcRect = { m_tilesetXOff * tset->tileWidth, m_tilesetYOff * tset->tileHeight, tset->tileWidth, tset->tileHeight };
				SDL_Rect dstRect = { static_cast<int>(m_isoPos.x), static_cast<int>(m_isoPos.y), tset->tileWidth, tset->tileHeight };
				m_occlusionRect = dstRect;

				m_occlusionRect.w -= m_occlModifierX;
				m_occlusionRect.h -= m_occlModifierY;
				m_occlusionRect.x += (m_occlModifierX / 2);
				m_occlusionRect.y += (m_occlModifierY / 2);

				// Draw the occlusion debug on the Debug Render Target
				if (DEBUG_OCCLUSION_ENABLED && (TestFlag(FCanOccludePlayer) || m_ID == Player::ENT_ID))
				{
					// Set debug target
					auto previousTarget = engine.GetRenderer().GetCurrentERenderTargetVal();
					engine.GetRenderer().SetRenderTarget(Renderer::ETargets::DEBUG_TARGET);

					// Adjust for zoom
					float zoomLevel = engine.GetGame().GetMainCamera()->GetZoom();
					engine.GetRenderer().SetScale(zoomLevel, zoomLevel); // TODO: this should not be here (probably in SetZoom with the main RenderTarget scale), we need to set the scale of the renderer one time and not for each debug draw

					// Draw
					engine.GetRenderer().DrawRect(&m_occlusionRect, colorYellow);

					// Restore previous target
					engine.GetRenderer().SetRenderTarget(previousTarget);
				}

				engine.GetRenderer().DrawImageDirectly(m_img->GetSrc(), &srcRect, &dstRect);
			}

			// Restore alpha mod and color mod
			SDL_SetTextureAlphaMod(m_img->GetSrc(), previousAlpha);
			SDL_SetTextureColorMod(m_img->GetSrc(), previousR, previousG, previousB);
		}

		if (HasCollider() && DEBUG_COLLIDER_ENABLED && (DEBUG_COLLIDER_LAYER == -1 || DEBUG_COLLIDER_LAYER == GetLayer() || m_ID == Player::ENT_ID))
			m_coll->DebugDraw();
	}

	//---------------------------------------------------------------------------------------------
	// Called after TransferToCellImmediately is executed, meant to be overriden by derived classes
	//---------------------------------------------------------------------------------------------
	inline void Entity::OnCellChanges()
	{

	}

}
}
