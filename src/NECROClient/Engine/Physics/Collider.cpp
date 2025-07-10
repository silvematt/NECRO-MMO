#include "Collider.h"

#include "NECROClient.h"

void Collider::Init(bool pEnabled, Entity* own, int x, int y, int w, int h)
{
	r.x = x;
	r.y = y;
	r.w = w;
	r.h = h;

	enabled = pEnabled;
	owner = own;
}

void Collider::SetOffset(float x, float y)
{
	collOffset.x = x;
	collOffset.y = y;
}

void Collider::Update()
{
	if (!enabled || !owner)
		return;

	float posX = owner->pos.x + collOffset.x;
	float posY = owner->pos.y + collOffset.y;

	// Center-align the collision rectangle on its entity's position:
	r.x = posX + (r.w / 2.0f);
	r.y = posY - (r.h / 2.0f);
}

void Collider::DebugDraw()
{
	auto previousTarget = engine.GetRenderer().GetCurrentERenderTargetVal();

	// Set debug target, update scale in base of the main camera zoom
	engine.GetRenderer().SetRenderTarget(NECRORenderer::ERenderTargets::DEBUG_TARGET);
	float zoomLevel = engine.GetGame().GetMainCamera()->GetZoom();
	engine.GetRenderer().SetScale(zoomLevel, zoomLevel); // TODO: this should not be here (probably in SetZoom with the main RenderTarget scale), we need to set the scale of the renderer one time and not for each debug draw
	
	Camera* c = engine.GetGame().GetMainCamera();
	engine.GetRenderer().DrawIsoBox(&r, colorPink, c->pos.x - HALF_CELL_WIDTH, c->pos.y - HALF_CELL_WIDTH, c->GetZoom()); // subtracting HALF_CELL_WIDTH corrects offset

	// Restore previous target
	engine.GetRenderer().SetRenderTarget(previousTarget);
}

// TODO: Implement intersection check instead of using SDL's.
bool Collider::TestIntersection(const Collider* other)
{
	return SDL_HasIntersection(&r, &other->r);
}
