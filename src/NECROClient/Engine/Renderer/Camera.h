#ifndef NECROCAMERA_H
#define NECROCAMERA_H

#include "Vector2.h"
#include "Entity.h"

namespace NECRO
{
namespace Client
{
	inline constexpr float CAMERA_DEFAULT_ZOOM = 1.0f;

	class Camera
	{
	private:
		const float MIN_ZOOM = 0.4f;
		const float MAX_ZOOM = 3.0f;

		// ------------------------------------------------------------------------------------------
		// zoomSizeX and zoomSizeY are used to keep the camera centered while zooming in/out.
		// 
		// They represent the size of the viewport on the X and Y axes after applying the zoom level
		// and are calculated as SCREEN_WIDTH / zoomLevel, SCREEN_HEIGHT / zoomLevel 
		// 
		// For example, if the zoom level is 2.0, zoomSizeX and zoomSizeY will represent half of the 
		// original screen width and height, showing half of the world.
		// 
		// When the zoom level changes, oldZoomSizeX and oldZoomSizeY (defined in the Update()) 
		// hold the previous viewport size, which is used to adjust the camera position to 
		// keep it centered correctly after the zoom.
		// ------------------------------------------------------------------------------------------
		float m_zoomSizeX;
		float m_zoomSizeY;

		float m_zoomLevel = 1.0f;
		float m_zoomSpeed = 0.2f;

		float m_panSpeed = 2.0f;

		bool  m_freeCamera = false;


		std::vector<Entity*> m_visibleStaticEntities;	// Entities, such as the terrain on layer0, that do not need any sorting to be drawn.
		std::vector<Entity*> m_visibleEntities;			// Entities that are in the camera-space, that will need to be drawn after sorting

	private:
		void		FreeMove();
		void		LockPlayerMove();

	public:
		Vector2		m_pos;

		void		Update();
		void		SetZoom(float newZoom);
		void		ResetZoom();
		float		GetZoom();

		void		AddToVisibleEntities(Entity* e);
		void		AddToVisibleStaticEntities(Entity* e);

		void		RenderVisibleEntities();
		void		RenderVisibleStaticEntities();

		Vector2		ScreenToWorld(const Vector2& point);	// ScreenToWorld while accounting for zoom
	};

	inline float Camera::GetZoom()
	{
		return m_zoomLevel;
	}

}
}

#endif
