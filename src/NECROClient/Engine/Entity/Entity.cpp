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
		ID = ENT_NEXT_ID;
		ENT_NEXT_ID++;

		img = nullptr;
		owner = nextOwner = nullptr;
		tilesetXOff = tilesetYOff = 0;
		gridPosX = gridPosY = 0;
		occlusionRect = { 0,0,0,0 };
		toRender = true;
	}

	Entity::Entity(Vector2 pInitialPos, Image* pImg)
	{
		ID = ENT_NEXT_ID;
		ENT_NEXT_ID++;

		pos = pInitialPos;
		gridPosX = pos.x / CELL_WIDTH;
		gridPosY = pos.y / CELL_HEIGHT;
		zPos = 0;
		toRender = true;

		SetImg(pImg);
	}

	//------------------------------------------------------------
	// Sets the Image of this Entity
	//------------------------------------------------------------
	void Entity::SetImg(Image* pImg)
	{
		img = pImg;
	}

	//------------------------------------------------------------
	// Sets the owner of this Entity
	//------------------------------------------------------------
	void Entity::SetOwner(Cell* c)
	{
		owner = c;
	}

	//------------------------------------------------------------
	// Clears the owner of this entity
	//------------------------------------------------------------
	void Entity::ClearOwner()
	{
		owner = nullptr;
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
		if (c == nullptr && nextOwner)
			c = nextOwner;

		// Remove Ptr from previous owner
		owner->RemoveEntityPtr(ID);

		// Update owner
		SetOwner(c);

		// Add Ptr to current owner
		c->AddEntityPtr(this);

		nextOwner = nullptr;

		OnCellChanges();
	}

	//-----------------------------------------------------------------------------------------------
	// TransferToCellQueue lets the world perform the transfer AFTER a world update completes.
	//-----------------------------------------------------------------------------------------------
	void Entity::TransferToCellQueue(Cell* c)
	{
		nextOwner = c;
		owner->GetWorld()->AddPendingEntityToTransfer(this);
	}

	//------------------------------------------------------------
	// Updates the Entity
	//------------------------------------------------------------
	void Entity::Update()
	{
		int oldGridPosX = gridPosX;
		int oldGridPosY = gridPosY;

		// Update grid position
		gridPosX = pos.x / CELL_WIDTH;
		gridPosY = pos.y / CELL_HEIGHT;

		// Update ISO coordinates
		NMath::CartToIso(pos.x / CELL_WIDTH, pos.y / CELL_HEIGHT, isoPos.x, isoPos.y);

		isoPos.x -= HALF_CELL_WIDTH;

		// Adjust isoX and isoY to the camera offset
		isoPos.x += engine.GetGame().GetMainCamera()->pos.x;
		isoPos.y += engine.GetGame().GetMainCamera()->pos.y;

		// Account for bottom-left origin
		if (!img->IsTileset())
			isoPos.y -= img->GetHeight();
		else
			isoPos.y -= img->GetTilesetHeight();

		// Account for the offset of the image
		isoPos.x += img->GetXOffset();
		isoPos.y += img->GetYOffset();

		// TODO: Naive solution? we can do topological sort instead of a single depth value if we encounter issues with this one for our goals, but it seems it can work
		depth = owner->GetCellX() + owner->GetCellY() + (LAYER_Z_COEFFICIENT * layer) + zPos; // zPos is used to position dynamic entities, like the player, or to offset static sprites.

		// If this entity has a collider and it is enabled, update it
		if (coll && coll->enabled)
			coll->Update();

		// Check for occlusion against player
		Entity* p = (Entity*)engine.GetGame().GetCurPlayer();
		if (p && p != this && TestFlag(Entity::Flags::FCanOccludePlayer))
		{
			// Check if this entity is close enough to the player to be worth testing intersection (first with gridPos, then with pos)
			if (abs(p->gridPosX - gridPosX) < ENTITY_OCCLUSION_TEST_X_DIFF && abs(p->gridPosY - gridPosY) < ENTITY_OCCLUSION_TEST_Y_DIFF &&
				(p->pos.x < this->pos.x || p->pos.y < this->pos.y) && SDL_HasIntersection(&occlusionRect, &p->occlusionRect))
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
		if (oldGridPosX != gridPosX || oldGridPosY != gridPosY)
		{
			nextOwner = owner->GetWorld()->GetCellAt(gridPosX, gridPosY);
			TransferToCellQueue(nextOwner); // will be done as soon as the world update is finished
		}
	}

	//------------------------------------------------------------
	// Looks at the lighting model and updates lightingColor to 
	// draw this entity
	//------------------------------------------------------------
	void Entity::UpdateLighting()
	{
		SDL_Color* cellColor = owner->GetLightingColor();
		lightingColor.r = cellColor->r * owner->GetLightingIntensity();
		lightingColor.g = cellColor->g * owner->GetLightingIntensity();
		lightingColor.b = cellColor->b * owner->GetLightingIntensity();
	}

	//------------------------------------------------------------
	// Draws the Entity
	//------------------------------------------------------------
	void Entity::Draw()
	{
		if (toRender)
		{
			UpdateLighting();

			// Save texture's alpha
			Uint8 previousAlpha = 0;
			SDL_GetTextureAlphaMod(img->GetSrc(), &previousAlpha);

			// Save texture's color
			Uint8 previousR, previousG, previousB;
			SDL_GetTextureColorMod(img->GetSrc(), &previousR, &previousG, &previousB);

			// Update alpha
			if (occludes)
				SDL_SetTextureAlphaMod(img->GetSrc(), OCCLUDED_SPRITE_ALPHA_VALUE);

			// Update Color with color data
			SDL_SetTextureColorMod(img->GetSrc(), lightingColor.r, lightingColor.g, lightingColor.b);

			if (!img->IsTileset())
			{
				SDL_Rect dstRect = { static_cast<int>(isoPos.x), static_cast<int>(isoPos.y), img->GetWidth(), img->GetHeight() };
				occlusionRect = dstRect;

				occlusionRect.w -= occlModifierX;
				occlusionRect.h -= occlModifierY;
				occlusionRect.x += (occlModifierX / 2);
				occlusionRect.y += (occlModifierY / 2);

				// Draw the occlusion debug on the Debug Render Target
				if (DEBUG_OCCLUSION_ENABLED && (TestFlag(FCanOccludePlayer) || ID == Player::ENT_ID))
				{
					// Set debug target
					auto previousTarget = engine.GetRenderer().GetCurrentERenderTargetVal();
					engine.GetRenderer().SetRenderTarget(Renderer::ERenderTargets::DEBUG_TARGET);

					// Adjust for zoom
					float zoomLevel = engine.GetGame().GetMainCamera()->GetZoom();
					engine.GetRenderer().SetScale(zoomLevel, zoomLevel); // TODO: this should not be here (probably in SetZoom with the main RenderTarget scale), we need to set the scale of the renderer one time and not for each debug draw

					// Draw
					engine.GetRenderer().DrawRect(&occlusionRect, colorRed);

					// Restore previous target
					engine.GetRenderer().SetRenderTarget(previousTarget);
				}

				engine.GetRenderer().DrawImageDirectly(img->GetSrc(), NULL, &dstRect);
			}
			else
			{
				Image::Tileset* tset = img->GetTileset();
				SDL_Rect srcRect = { tilesetXOff * tset->tileWidth, tilesetYOff * tset->tileHeight, tset->tileWidth, tset->tileHeight };
				SDL_Rect dstRect = { static_cast<int>(isoPos.x), static_cast<int>(isoPos.y), tset->tileWidth, tset->tileHeight };
				occlusionRect = dstRect;

				occlusionRect.w -= occlModifierX;
				occlusionRect.h -= occlModifierY;
				occlusionRect.x += (occlModifierX / 2);
				occlusionRect.y += (occlModifierY / 2);

				// Draw the occlusion debug on the Debug Render Target
				if (DEBUG_OCCLUSION_ENABLED && (TestFlag(FCanOccludePlayer) || ID == Player::ENT_ID))
				{
					// Set debug target
					auto previousTarget = engine.GetRenderer().GetCurrentERenderTargetVal();
					engine.GetRenderer().SetRenderTarget(Renderer::ERenderTargets::DEBUG_TARGET);

					// Adjust for zoom
					float zoomLevel = engine.GetGame().GetMainCamera()->GetZoom();
					engine.GetRenderer().SetScale(zoomLevel, zoomLevel); // TODO: this should not be here (probably in SetZoom with the main RenderTarget scale), we need to set the scale of the renderer one time and not for each debug draw

					// Draw
					engine.GetRenderer().DrawRect(&occlusionRect, colorYellow);

					// Restore previous target
					engine.GetRenderer().SetRenderTarget(previousTarget);
				}

				engine.GetRenderer().DrawImageDirectly(img->GetSrc(), &srcRect, &dstRect);
			}

			// Restore alpha mod and color mod
			SDL_SetTextureAlphaMod(img->GetSrc(), previousAlpha);
			SDL_SetTextureColorMod(img->GetSrc(), previousR, previousG, previousB);
		}

		if (HasCollider() && DEBUG_COLLIDER_ENABLED && (DEBUG_COLLIDER_LAYER == -1 || DEBUG_COLLIDER_LAYER == GetLayer() || ID == Player::ENT_ID))
			coll->DebugDraw();
	}

	//---------------------------------------------------------------------------------------------
	// Called after TransferToCellImmediately is executed, meant to be overriden by derived classes
	//---------------------------------------------------------------------------------------------
	inline void Entity::OnCellChanges()
	{

	}

}
}
