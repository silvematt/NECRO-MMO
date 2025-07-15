#include "Collider.h"

#include "NECROEngine.h"

namespace NECRO
{
namespace Client
{
	void Collider::Init(bool pEnabled, Entity* own, int x, int y, int w, int h)
	{
		m_r.x = x;
		m_r.y = y;
		m_r.w = w;
		m_r.h = h;

		m_enabled = pEnabled;
		m_owner = own;
	}

	void Collider::SetOffset(float x, float y)
	{
		m_collOffset.x = x;
		m_collOffset.y = y;
	}

	void Collider::Update()
	{
		if (!m_enabled || !m_owner)
			return;

		float posX = m_owner->m_pos.x + m_collOffset.x;
		float posY = m_owner->m_pos.y + m_collOffset.y;

		// Center-align the collision rectangle on its entity's position:
		m_r.x = posX + (m_r.w / 2.0f);
		m_r.y = posY - (m_r.h / 2.0f);
	}

	void Collider::DebugDraw()
	{
		auto previousTarget = engine.GetRenderer().GetCurrentERenderTargetVal();

		// Set debug target, update scale in base of the main camera zoom
		engine.GetRenderer().SetRenderTarget(Renderer::ETargets::DEBUG_TARGET);
		float zoomLevel = engine.GetGame().GetMainCamera()->GetZoom();
		engine.GetRenderer().SetScale(zoomLevel, zoomLevel); // TODO: this should not be here (probably in SetZoom with the main RenderTarget scale), we need to set the scale of the renderer one time and not for each debug draw

		Camera* c = engine.GetGame().GetMainCamera();
		engine.GetRenderer().DrawIsoBox(&m_r, colorPink, c->m_pos.x - HALF_CELL_WIDTH, c->m_pos.y - HALF_CELL_WIDTH, c->GetZoom()); // subtracting HALF_CELL_WIDTH corrects offset

		// Restore previous target
		engine.GetRenderer().SetRenderTarget(previousTarget);
	}

	bool Collider::TestIntersection(const Collider* other)
	{
		return SDL_HasIntersection(&m_r, &other->m_r);
	}

}
}
