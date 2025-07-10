#ifndef NECROCOLLIDER_H
#define NECROCOLLIDER_H

#include "SDL.h"

#include "Vector2.h"
#include "Image.h"

class Entity;

//-------------------------------------------------------------------------------------
// A collider is attached to an Entity (or classes derived from Entity) and is used to
// define the collision box of the entities.
//-------------------------------------------------------------------------------------
class Collider
{
public:
	Collider() = default;

public:
	void Init(bool pEnabled, Entity* own, int x, int y, int w, int h);
	void Update();

private:
	Entity* owner;

	Vector2 collOffset;			// the offset of the collision box 'r' added to r.x and r.y (which represent the position of the entity owning this collider)
	SDL_Rect r;

	float isoPosX, isoPosY;

public:
	bool enabled = false;

	void			SetOffset(float x, float y);
	void			DebugDraw();
	const SDL_Rect*	GetRect() const;

	bool			TestIntersection(const Collider* other);
};


inline const SDL_Rect* Collider::GetRect() const
{
	return &r;
}

#endif
