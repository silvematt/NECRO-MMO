#include "Camera.h"

#include "NECROEngine.h"
#include "Player.h"

namespace NECRO
{
namespace Client
{
	// ----------------------------------------------------------------------------------------------------
	// Sets the camera zoom level
	// ----------------------------------------------------------------------------------------------------
	void Camera::SetZoom(float newZoom)
	{
		m_zoomLevel = newZoom;
		m_zoomLevel = SDL_clamp(m_zoomLevel, MIN_ZOOM, MAX_ZOOM);

		m_zoomSizeX = SCREEN_WIDTH / m_zoomLevel;
		m_zoomSizeY = SCREEN_HEIGHT / m_zoomLevel;
		engine.GetRenderer().SetScale(m_zoomLevel, m_zoomLevel);
	}

	// ----------------------------------------------------------------------------------------------------
	// Resets the camera zoom level to the default vlaue
	// ----------------------------------------------------------------------------------------------------
	void Camera::ResetZoom()
	{
		m_zoomLevel = CAMERA_DEFAULT_ZOOM;
		m_zoomLevel = SDL_clamp(m_zoomLevel, MIN_ZOOM, MAX_ZOOM);

		m_zoomSizeX = SCREEN_WIDTH / m_zoomLevel;
		m_zoomSizeY = SCREEN_HEIGHT / m_zoomLevel;
		engine.GetRenderer().SetScale(m_zoomLevel, m_zoomLevel);
	}

	// ----------------------------------------------------------------------------------------------------
	// From a point on the screen returns the point in the world
	// ----------------------------------------------------------------------------------------------------
	Vector2 Camera::ScreenToWorld(const Vector2& point)
	{
		Vector2 worldPoint;

		// Adjust for offset and zoom level
		worldPoint.x = (point.x - m_pos.x * m_zoomLevel) / m_zoomLevel;
		worldPoint.y = (point.y - m_pos.y * m_zoomLevel) / m_zoomLevel;
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
		m_visibleEntities.clear();
		m_visibleStaticEntities.clear();

		if (engine.GetInput().GetKeyDown(SDL_SCANCODE_C) && !engine.GetConsole().IsOpen())
			m_freeCamera = !m_freeCamera;

		if (m_freeCamera)
			FreeMove();
		else
			LockPlayerMove();
	}

	void Camera::LockPlayerMove()
	{
		// Keep camera centered
		float oldX = HALF_SCREEN_WIDTH / m_zoomLevel;
		float oldY = HALF_SCREEN_HEIGHT / m_zoomLevel;

		// Hold the zoom size before zoom change
		float oldZoomSizeX = m_zoomSizeX;
		float oldZoomSizeY = m_zoomSizeY;

		if (engine.GetInput().GetMouseScroll() > 0)			// Zoom in		
			SetZoom(m_zoomLevel + m_zoomSpeed);
		else if (engine.GetInput().GetMouseScroll() < 0)	// Zoom out
			SetZoom(m_zoomLevel - m_zoomSpeed);

		// Camera movement
		float deltaX = 0.0f;
		float deltaY = 0.0f;

		// Delta the player position
		Player* player = engine.GetGame().GetCurPlayer();
		if (player)
		{
			float sx, sy;
			NMath::CartToIso(player->m_pos.x / CELL_WIDTH, player->m_pos.y / CELL_HEIGHT, sx, sy);

			deltaX = sx;
			deltaY = sy - HALF_PLAYER_HEIGHT;
		}
		else
		{
			m_freeCamera = true; // if the player is not there, go in free camera
			return;
		}

		// Update position, subtracting ((oldZoomSizeX / 2) - (zoomSizeX / 2)) allows us to keep the camera centered after zooming
		// It adjusts the camera position by half of the difference in each dimension to keep the view centered.
		m_pos.x = oldX - ((oldZoomSizeX / 2) - (m_zoomSizeX / 2)) - deltaX;
		m_pos.y = oldY - ((oldZoomSizeY / 2) - (m_zoomSizeY / 2)) - deltaY;
	}

	void Camera::FreeMove()
	{
		float oldX = m_pos.x;
		float oldY = m_pos.y;

		// Hold the zoom size before zoom change
		float oldZoomSizeX = m_zoomSizeX;
		float oldZoomSizeY = m_zoomSizeY;

		if (engine.GetInput().GetMouseScroll() > 0)			// Zoom in		
			SetZoom(m_zoomLevel + m_zoomSpeed);
		else if (engine.GetInput().GetMouseScroll() < 0)	// Zoom out
			SetZoom(m_zoomLevel - m_zoomSpeed);

		// Camera movement
		float deltaX = 0.0f;
		float deltaY = 0.0f;

		if (engine.GetInput().GetMouseHeld(static_cast<SDL_Scancode>(SDL_BUTTON_MIDDLE)))
		{
			// TODO: fix mouseMotion at different framerates
			Vector2 mouseMotion = engine.GetInput().GetMouseMotionThisFrame();
			deltaX += mouseMotion.x * m_panSpeed * engine.GetDeltaTime();
			deltaY += mouseMotion.y * m_panSpeed * engine.GetDeltaTime();
		}

		// Update position, subtracting ((oldZoomSizeX / 2) - (zoomSizeX / 2)) allows us to keep the camera centered after zooming
		// It adjusts the camera position by half of the difference in each dimension to keep the view centered.
		m_pos.x = oldX - ((oldZoomSizeX / 2) - (m_zoomSizeX / 2)) + deltaX;
		m_pos.y = oldY - ((oldZoomSizeY / 2) - (m_zoomSizeY / 2)) + deltaY;
	}


	void Camera::AddToVisibleEntities(Entity* e)
	{
		m_visibleEntities.push_back(e);
	}

	void Camera::AddToVisibleStaticEntities(Entity* e)
	{
		m_visibleStaticEntities.push_back(e);
	}

	// TODO make this a template in Utility class
	void QuickSort(std::vector<Entity*>& entities, int left, int right)
	{
		int i = left, j = right;
		float pivot = entities[(left + right) / 2]->m_depth;

		while (i <= j)
		{
			while (entities[i]->m_depth < pivot)
				i++;
			while (entities[j]->m_depth > pivot)
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
		engine.GetRenderer().SetRenderTarget(Renderer::ETargets::MAIN_TARGET);

		// Render the static entities
		RenderVisibleStaticEntities();

		// Sort and draw the visibleEntities list
		if (!m_visibleEntities.empty())
		{
			QuickSort(m_visibleEntities, 0, m_visibleEntities.size() - 1);

			// Draw them
			for (auto& ent : m_visibleEntities)
				if (ent)
					ent->Draw();
		}
	}

	void Camera::RenderVisibleStaticEntities()
	{
		// We should be already on the main rendering target here

		for (auto& ent : m_visibleStaticEntities)
			if (ent)
				ent->Draw();
	}

}
}
