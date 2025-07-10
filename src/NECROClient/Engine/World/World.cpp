#include "World.h"

#include "NECROClient.h"
#include "Entity.h"
#include "Player.h"
#include "AI.h"

Camera* curCamera;

//------------------------------------------------------------
// Initializes the World
//------------------------------------------------------------
void World::InitializeWorld()
{
	worldCursor = nullptr;
	worldCursorEditTexture = engine.GetAssetsManager().GetImage("tile_highlighted.png")->GetSrc();
	worldCursorPlayTexture = engine.GetAssetsManager().GetImage("tile_highlighted_play.png")->GetSrc();

	// Add a player, just for testing (ID = 0)
	std::unique_ptr<Player> p(new Player());
	p->SetImg(engine.GetAssetsManager().GetImage("player_war_idle.png"));
	p->pos = Vector2(static_cast<float>(22 * CELL_WIDTH), static_cast<float>(23 * CELL_HEIGHT));
	p->zPos = 100.01f; // Player Z placement is controlled by zPos, while for static entities (map-defined) we use layers
	p->SetLayer(0);
	p->Init();
	p->SetFlag(Entity::Flags::Dynamic);
	Player::ENT_ID = p->GetID();
	engine.GetGame().SetCurPlayer(p.get());
	AddEntity(std::move(p));

	map.LoadMap("world.nmap");

	// TEST: Add AI for testing
	std::unique_ptr<AI> ai(new AI());
	ai->SetImg(engine.GetAssetsManager().GetImage("skeleton_war_idle.png"));
	ai->pos = Vector2(static_cast<float>(29 * CELL_WIDTH), static_cast<float>(8 * CELL_HEIGHT));
	ai->zPos = 100.01f;
	ai->SetLayer(0);
	ai->Init();
	ai->SetFlag(Entity::Flags::Dynamic);
	AddEntity(std::move(ai));

	for(int x = 0; x < WORLD_WIDTH; x++)
		for (int y = 0; y < WORLD_HEIGHT; y++)
		{
			// Initialize the Cells
			Cell& currentCell = worldmap[x][y];

			currentCell.SetWorld(this);
			currentCell.SetCellCoordinates(x, y);
		}

	// Set camera
	curCamera = engine.GetGame().GetMainCamera();
	curCamera->pos.x = SCREEN_WIDTH  / 2;
	curCamera->pos.y = SCREEN_HEIGHT / 2;
	curCamera->SetZoom(CAMERA_DEFAULT_ZOOM);
}

//------------------------------------------------------------------------------
// Updates visibleMinMax variables based on curCamera position and zoom
//------------------------------------------------------------------------------
void World::UpdateVisibleCoords()
{
	// Get tile at the center of the screen
	Vector2 midTilePos = engine.GetGame().GetMainCamera()->ScreenToWorld(Vector2(HALF_SCREEN_WIDTH, HALF_SCREEN_HEIGHT));
	midTilePos.Clamp(0, WORLD_WIDTH - 1, 0, WORLD_HEIGHT - 1);
	Cell* midTile = &worldmap[(int)midTilePos.x][(int)midTilePos.y];

	// Compute visible min/max 
	float curCameraZoom = curCamera->GetZoom();
	visibleMinX = (int)(midTile->GetCellX() - roundf(((HALF_SCREEN_WIDTH / HALF_CELL_WIDTH) / curCameraZoom))) - VISIBLE_X_PLUS_OFFSET;
	visibleMaxX = (int)(midTile->GetCellX() + roundf(((HALF_SCREEN_WIDTH / HALF_CELL_WIDTH) / curCameraZoom))) + VISIBLE_X_PLUS_OFFSET;
	visibleMinY = (int)(midTile->GetCellY() - roundf(((HALF_SCREEN_HEIGHT / HALF_CELL_HEIGHT) / curCameraZoom))) - VISIBLE_X_PLUS_OFFSET;
	visibleMaxY = (int)(midTile->GetCellY() + roundf(((HALF_SCREEN_HEIGHT / HALF_CELL_HEIGHT) / curCameraZoom))) + VISIBLE_X_PLUS_OFFSET;

	// Clamp
	visibleMinX = SDL_clamp(visibleMinX, 0, WORLD_WIDTH-1);
	visibleMaxX = SDL_clamp(visibleMaxX, 0, WORLD_WIDTH-1);
	visibleMinY = SDL_clamp(visibleMinY, 0, WORLD_HEIGHT-1);
	visibleMaxY = SDL_clamp(visibleMaxY, 0, WORLD_HEIGHT-1);
}

//------------------------------------------------------------
// Updates the world
//------------------------------------------------------------
void World::Update()
{
	UpdateVisibleCoords();

	ComputeOnSelectedCell();

	// Reset the lighting for all visible cells, it is re-calculated later
	ResetLighting();

	// Update cells only in the visible rect : TODO: For updating entities (which are updated inside the Cell.Update() we may want to have some entities to update offscreen, like roaming bosses
	//										   We can have a list of these entities instead of just updating the visible cells (the list will contain the entities inside the visible cells too)
	entitiesWaitingForTransfer.clear();
	for (int x = visibleMinX; x < visibleMaxX; x++)
		for (int y = visibleMinY; y < visibleMaxY; y++)
		{
			// Update the cell
			Cell& currentCell = worldmap[x][y];
			currentCell.Update();
		}

	TransferPendingEntities();
}

//------------------------------------------------------------
// Draws the world. 
//------------------------------------------------------------
void World::Draw()
{
	// Draw the debug-related world sprites and add all the entities that needs to be drawn to the camera list
	engine.GetRenderer().SetRenderTarget(NECRORenderer::DEBUG_TARGET);
	engine.GetRenderer().SetScale(curCamera->GetZoom(), curCamera->GetZoom()); // TODO: this should not be here (probably in SetZoom with the main RenderTarget scale), we need to set the scale of the renderer one time and not for each debug draw

	// Draw interact cursor before the actual entity
	if (worldCursor && engine.GetGame().GetCurMode() == GameMode::PLAY_MODE && canInteract)
		engine.GetRenderer().DrawImageDirectly(worldCursorPlayTexture, NULL, &worldCursor->GetDstRect());

	// Add visible entities to the camera list
	for (int x = visibleMinX; x < visibleMaxX; x++)
		for (int y = visibleMinY; y < visibleMaxY; y++)
		{
			Cell& currentCell = worldmap[x][y];
			currentCell.AddEntitiesAsVisible();
		}

	// Draw the edit cursor on top of entities
	if(worldCursor && engine.GetGame().GetCurMode() == GameMode::EDIT_MODE)
		engine.GetRenderer().DrawImageDirectly(worldCursorEditTexture, NULL, &worldCursor->GetDstRect());

	DrawUI();
}

//------------------------------------------------------------
// Allows to perform actions on the mouse-selected cell 
// Must be called from Update()
//------------------------------------------------------------
void World::ComputeOnSelectedCell()
{
	// Compute selected cell
	int selectedCellX = 0, selectedCellY = 0;

	Vector2 mousePos(static_cast<float>(engine.GetInput().GetMouseX()), static_cast<float>(engine.GetInput().GetMouseY()));
	Vector2 mouseInWorld = curCamera->ScreenToWorld(mousePos);

	selectedCellX = mouseInWorld.x;
	selectedCellY = mouseInWorld.y;

	if (selectedCellX >= 0 && selectedCellX < WORLD_WIDTH && selectedCellY >= 0 && selectedCellY < WORLD_HEIGHT)
	{
		worldCursor = &worldmap[selectedCellX][selectedCellY];

		// IF IN EDIT MDOE
		if (engine.GetGame().GetCurMode() == GameMode::EDIT_MODE)
		{
			if (engine.GetInput().GetMouseDown(static_cast<SDL_Scancode>(SDL_BUTTON_LEFT)))
			{
				if (worldCursor->GetEntitiesPtrSize() != 0)
				{
					RemoveEntity(worldCursor->GetEntityPtrAt(0)->GetID());
				}
				else
				{
					std::unique_ptr<Entity> tree = Prefab::InstantiatePrefab("campfire01", Vector2(worldCursor->GetCellX() * CELL_WIDTH, worldCursor->GetCellY() * CELL_HEIGHT));
					if (tree)
						AddEntity(std::move(tree));
				}
			}
			else if (engine.GetInput().GetMouseDown(static_cast<SDL_Scancode>(SDL_BUTTON_RIGHT)))
			{
				if (worldCursor->GetEntitiesPtrSize() != 0)
				{
					RemoveEntity(worldCursor->GetEntityPtrAt(0)->GetID());
				}
				else
				{
					std::unique_ptr<Entity> tree = Prefab::InstantiatePrefab("tree01", Vector2(worldCursor->GetCellX() * CELL_WIDTH, worldCursor->GetCellY() * CELL_HEIGHT));
					if (tree)
						AddEntity(std::move(tree));
				}
			}
		}
		// Handle PlayMode interaction
		else if (engine.GetGame().GetCurMode() == GameMode::PLAY_MODE)
		{
			canInteract = false;
			if (worldCursor->GetEntitiesPtrSize() != 0)
			{
				// TODO_ make proper entity selection instead of selecting the first one,
				// If we end up with possible multiple interactable entities on the same cell, we can have an UI box that allows us to select which one we want to interact with
				Entity* e = worldCursor->GetEntityPtrAt(0);
				if (e->HasInteractable())
				{
					// Check if the player is closer enough to interact with this interactable
					Interactable* i = e->GetInteractable(0); // get the first interactable to get the gridDistanceInteraction, it must be the same on every interactables anyways
					Player* p = engine.GetGame().GetCurPlayer();

					// If player is valid
					if(p)
						if (NECRO_MAX(abs(p->gridPosX - e->gridPosX), abs(p->gridPosY - e->gridPosY)) <= i->gridDistanceInteraction || // check distance
							i->gridDistanceInteraction == 0) // 0 means interact from any distance
						{
							canInteract = true;

							if (engine.GetInput().GetMouseDown(static_cast<SDL_Scancode>(SDL_BUTTON_LEFT)))
							{
								// Call Interact() on all interactables
								for (int i = 0; i < e->GetInteractablesSize(); i++)
									e->GetInteractable(i)->Interact();
							}
						}
				}
			}
		}
	}
	else
		worldCursor = nullptr;
}

//------------------------------------------------------------
// Draws interface (FPS, Selected Cell etc) 
//------------------------------------------------------------
void World::DrawUI()
{
	NECRORenderer& renderer = engine.GetRenderer();
	// Draw the UI on the overlay target
	renderer.SetRenderTarget(NECRORenderer::ERenderTargets::OVERLAY_TARGET);

	// Draw text
	
	// FPS
	std::string textFPS = "FPS: " + std::to_string((int)engine.GetFPS());
	renderer.DrawTextDirectly(engine.GetAssetsManager().GetFont("defaultFont"), textFPS.c_str(), 10, 10, colorRed);

	// Draw selected cell
	std::string textSelCell = "Selected Cell: ";
	if (worldCursor)
		textSelCell = textSelCell + "(" + std::to_string(worldCursor->GetCellX()) + ", " + std::to_string(worldCursor->GetCellY()) + ")";
	renderer.DrawTextDirectly(engine.GetAssetsManager().GetFont("defaultFont"), textSelCell.c_str(), 10, 50, colorRed);

	// Draw current mode
	std::string textCurrentMode = "Mode: " + GameModeMap[engine.GetGame().GetCurMode()];
	renderer.DrawTextDirectly(engine.GetAssetsManager().GetFont("defaultFont"), textCurrentMode.c_str(), SCREEN_WIDTH - 300, 10, colorRed);

	// Draw player Z layer
	Player* p = engine.GetGame().GetCurPlayer();
	if (p)
	{
		std::string playerZLayer = "Player Z Layer: " + std::to_string(p->GetLayerFromZPos());
		renderer.DrawTextDirectly(engine.GetAssetsManager().GetFont("defaultFont"), playerZLayer.c_str(), SCREEN_WIDTH - 300, 50, colorRed);
	}

}

//------------------------------------------------------------------------
// Returns a Cell* at the coordinates specified as arguments of this call
//------------------------------------------------------------------------
Cell* World::GetCellAt(int x, int y)
{
	if (x >= 0 && x < WORLD_WIDTH && y >= 0 && y < WORLD_HEIGHT)
		return &worldmap[x][y];
	else
		return nullptr;
}

//-----------------------------------------------------------------------
// Adds the Entity to the entities map, transferring ownership
//-----------------------------------------------------------------------
void World::AddEntity(std::unique_ptr<Entity>&& e)
{
	// Calculate grid position
	e->gridPosX = e->pos.x / CELL_WIDTH;
	e->gridPosY = e->pos.y / CELL_HEIGHT;

	// Check if it is in bound
	if (e->gridPosX >= 0 && e->gridPosX < WORLD_WIDTH && e->gridPosY >= 0 && e->gridPosY < WORLD_HEIGHT)
	{
		// Add a ptr into the Cell
		worldmap[e->gridPosX][e->gridPosY].AddEntityPtr(e.get());

		// Add entity in the world map
		allEntities.insert({ e->GetID(), std::move(e) });
	}
	else
	{
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "World::AddEntity(): gridPos is out of bounds. Entity will not be added to map.\n");
	}
}

void World::RemoveEntity(uint32_t atID)
{
	allEntities.at(atID)->GetOwner()->RemoveEntityPtr(atID);
	allEntities.erase(atID);
}

void World::TransferPendingEntities()
{
	for (int i = 0; i < entitiesWaitingForTransfer.size(); i++)
	{
		// nullptr as argument means Entity->nextOwner
		entitiesWaitingForTransfer[i]->TransferToCellImmediately(nullptr);
	}
}

void World::AddPendingEntityToTransfer(Entity* e)
{
	entitiesWaitingForTransfer.push_back(e);
}

void World::ResetLighting()
{
	for (int x = visibleMinX; x < visibleMaxX; x++)
		for (int y = visibleMinY; y < visibleMaxY; y++)
		{
			// Reset lighting for the visible world to the world base light and color
			Cell& currentCell = worldmap[x][y];
			currentCell.SetLightingIntensity(baseLight);
			currentCell.SetLightingColor(baseLightColor.r, baseLightColor.g, baseLightColor.b);
		}
}

bool World::IsInWorldBounds(int x, int y)
{
	return (x >= 0 && x < WORLD_WIDTH && y >= 0 && y < WORLD_HEIGHT);
}

