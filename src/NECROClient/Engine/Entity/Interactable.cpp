#include "Interactable.h"

#include "World.h"
#include "Cell.h"
#include "Light.h"
#include "Collider.h"
#include "Animator.h"

void Interactable::Interact()
{
	SDL_Log("Entity[%d] has been interacted with. TYPE[%d]", owner->GetID(),  static_cast<int>(type));

	switch (type)
	{
		case InteractType::DESTROY:
			INDestroy();
			break;

		case InteractType::TOGGLE_LIGHT:
			INToggleLight();
			break;

		case InteractType::TOGGLE_COLL:
			INToggleColl();
			break;

		case InteractType::PLAY_ANIM:
			INPlayAnim(parStr);
			break;

		case InteractType::MOVE:
			INMove(parFloat1, parFloat2);
			break;

		case InteractType::MOVE_RELATIVE:
			INMoveRelative(parFloat1, parFloat2);
			break;

		default:
			break;
	}
}

void Interactable::INDestroy()
{
	owner->GetOwner()->GetWorld()->RemoveEntity(owner->GetID());
}

void Interactable::INToggleLight()
{
	if (owner->HasLight())
		owner->GetLight()->enabled = !owner->GetLight()->enabled;
}

void Interactable::INToggleColl()
{
	if (owner->HasCollider())
		owner->GetCollider()->enabled = !owner->GetCollider()->enabled;
}

void Interactable::INPlayAnim(std::string state)
{
	if (owner->HasAnimator())
		owner->GetAnimator()->Play(state);
}

void Interactable::INMove(float x, float y)
{
	owner->pos.x = x;
	owner->pos.y = y;
}

void Interactable::INMoveRelative(float x, float y)
{
	owner->pos.x += x;
	owner->pos.y += y;
}
