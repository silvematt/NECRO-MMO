#include "Camera.h"

#include "NECROClient.h"
#include "Player.h"

// ----------------------------------------------------------------------------------------------------
// Sets the camera zoom level
// ----------------------------------------------------------------------------------------------------
void Camera::SetZoom(float newZoom)
{
	zoomLevel = newZoom;
	zoomLevel = SDL_clamp(zoomLevel, MIN_ZOOM, MAX_ZOOM);

	zoomSizeX = SCREEN_WIDTH / zoomLevel;
	zoomSizeY = SCREEN_HEIGHT / zoomLevel;
	engine.GetRenderer().SetScale(zoomLevel, zoomLevel);
}

// ----------------------------------------------------------------------------------------------------
// Resets the camera zoom level to the default vlaue
// ----------------------------------------------------------------------------------------------------
void Camera::ResetZoom()
{
	zoomLevel = CAMERA_DEFAULT_ZOOM;
	zoomLevel = SDL_clamp(zoomLevel, MIN_ZOOM, MAX_ZOOM);

	zoomSizeX = SCREEN_WIDTH / zoomLevel;
	zoomSizeY = SCREEN_HEIGHT / zoomLevel;
	engine.GetRenderer().SetScale(zoomLevel, zoomLevel);
}

// ----------------------------------------------------------------------------------------------------
// From a point on the screen returns the point in the world
// ----------------------------------------------------------------------------------------------------
Vector2 Camera::ScreenToWorld(const Vector2& point)
{
	Vector2 worldPoint;

	// Adjust for offset and zoom level
	worldPoint.x = (point.x - pos.x * zoomLevel) / zoomLevel;
	worldPoint.y = (point.y - pos.y * zoomLevel) / zoomLevel;
	worldPoint.y += CELL_HEIGHT; // account for bottom-left origin

	// Convert to world coordinates
	NMath::IsoToCart(worldPoint.x, worldPoint.y, worldPoint.x, worldPoint.y);

	return worldPoint;
}

// ----------------------------------------------------------------------------------------------------
// Update camera
// ----------------------------------------------------------------------------------------------------
void Camera::Update()
{
	visibleEntities.clear();
	visibleStaticEntities.clear();

	if (engine.GetInput().GetKeyDown(SDL_SCANCODE_C) && !engine.GetConsole().IsOpen())
		freeCamera = !freeCamera;

	if (freeCamera)
		FreeMove();
	else
		LockPlayerMove();
}

void Camera::LockPlayerMove()
{
	// Keep camera centered
	float oldX = HALF_SCREEN_WIDTH / zoomLevel;
	float oldY = HALF_SCREEN_HEIGHT / zoomLevel;

	// Hold the zoom size before zoom change
	float oldZoomSizeX = zoomSizeX;
	float oldZoomSizeY = zoomSizeY;

	if (engine.GetInput().GetMouseScroll() > 0)			// Zoom in		
		SetZoom(zoomLevel + zoomSpeed);
	else if (engine.GetInput().GetMouseScroll() < 0)	// Zoom out
		SetZoom(zoomLevel - zoomSpeed);

	// Camera movement
	float deltaX = 0.0f;
	float deltaY = 0.0f;

	// Delta the player position
	Player* player = engine.GetGame().GetCurPlayer();
	if (player)
	{
		float sx, sy;
		NMath::CartToIso(player->pos.x / CELL_WIDTH, player->pos.y / CELL_HEIGHT, sx, sy);

		deltaX = sx;
		deltaY = sy - HALF_PLAYER_HEIGHT;
	}
	else
	{
		freeCamera = true; // if the player is not there, go in free camera
		return;
	}

	// Update position, subtracting ((oldZoomSizeX / 2) - (zoomSizeX / 2)) allows us to keep the camera centered after zooming
	// It adjusts the camera position by half of the difference in each dimension to keep the view centered.
	pos.x = oldX - ((oldZoomSizeX / 2) - (zoomSizeX / 2)) - deltaX;
	pos.y = oldY - ((oldZoomSizeY / 2) - (zoomSizeY / 2)) - deltaY;
}

void Camera::FreeMove()
{
	float oldX = pos.x;
	float oldY = pos.y;

	// Hold the zoom size before zoom change
	float oldZoomSizeX = zoomSizeX;
	float oldZoomSizeY = zoomSizeY;

	if (engine.GetInput().GetMouseScroll() > 0)			// Zoom in		
		SetZoom(zoomLevel + zoomSpeed);
	else if (engine.GetInput().GetMouseScroll() < 0)	// Zoom out
		SetZoom(zoomLevel - zoomSpeed);

	// Camera movement
	float deltaX = 0.0f;
	float deltaY = 0.0f;

	if (engine.GetInput().GetMouseHeld(static_cast<SDL_Scancode>(SDL_BUTTON_MIDDLE)))
	{
		// TODO: fix mouseMotion at different framerates
		Vector2 mouseMotion = engine.GetInput().GetMouseMotionThisFrame();
		deltaX += mouseMotion.x * panSpeed * engine.GetDeltaTime();
		deltaY += mouseMotion.y * panSpeed * engine.GetDeltaTime();
	}

	// Update position, subtracting ((oldZoomSizeX / 2) - (zoomSizeX / 2)) allows us to keep the camera centered after zooming
	// It adjusts the camera position by half of the difference in each dimension to keep the view centered.
	pos.x = oldX - ((oldZoomSizeX / 2) - (zoomSizeX / 2)) + deltaX;
	pos.y = oldY - ((oldZoomSizeY / 2) - (zoomSizeY / 2)) + deltaY;
}


void Camera::AddToVisibleEntities(Entity* e)
{
	visibleEntities.push_back(e);
}

void Camera::AddToVisibleStaticEntities(Entity* e)
{
	visibleStaticEntities.push_back(e);
}

// TODO make this a template in Utility class
void QuickSort(std::vector<Entity*>& entities, int left, int right)
{
	int i = left, j = right;
	float pivot = entities[(left + right) / 2]->depth;

	while (i <= j)
	{
		while (entities[i]->depth < pivot)
			i++;
		while (entities[j]->depth > pivot)
			j--;
		if (i <= j)
		{
			std::swap(entities[i], entities[j]);
			i++;
			j--;
		}
	}

	if (left < j)
		QuickSort(entities, left, j);
	if (i < right)
		QuickSort(entities, i, right);
}

void Camera::RenderVisibleEntities()
{
	engine.GetRenderer().SetRenderTarget(NECRORenderer::MAIN_TARGET);

	// Render the static entities
	RenderVisibleStaticEntities();

	// Sort and draw the visibleEntities list
	if (!visibleEntities.empty())
	{
		QuickSort(visibleEntities, 0, visibleEntities.size() - 1);

		// Draw them
		for (auto& ent : visibleEntities)
			if (ent)
				ent->Draw();
	}
}

void Camera::RenderVisibleStaticEntities()
{
	// We should be already on the main rendering target here

	for (auto& ent : visibleStaticEntities)
		if (ent)
			ent->Draw();
}