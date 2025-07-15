#include "Interactable.h"

#include "World.h"
#include "Cell.h"
#include "Light.h"
#include "Collider.h"
#include "Animator.h"

namespace NECRO
{
namespace Client
{
	void Interactable::Interact()
	{
		SDL_Log("Entity[%d] has been interacted with. TYPE[%d]", m_owner->GetID(), static_cast<int>(m_type));

		switch (m_type)
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
			INPlayAnim(m_parStr);
			break;

		case InteractType::MOVE:
			INMove(m_parFloat1, m_parFloat2);
			break;

		case InteractType::MOVE_RELATIVE:
			INMoveRelative(m_parFloat1, m_parFloat2);
			break;

		default:
			break;
		}
	}

	void Interactable::INDestroy()
	{
		m_owner->GetOwner()->GetWorld()->RemoveEntity(m_owner->GetID());
	}

	void Interactable::INToggleLight()
	{
		if (m_owner->HasLight())
			m_owner->GetLight()->m_enabled = !m_owner->GetLight()->m_enabled;
	}

	void Interactable::INToggleColl()
	{
		if (m_owner->HasCollider())
			m_owner->GetCollider()->m_enabled = !m_owner->GetCollider()->m_enabled;
	}

	void Interactable::INPlayAnim(std::string state)
	{
		if (m_owner->HasAnimator())
			m_owner->GetAnimator()->Play(state);
	}

	void Interactable::INMove(float x, float y)
	{
		m_owner->m_pos.x = x;
		m_owner->m_pos.y = y;
	}

	void Interactable::INMoveRelative(float x, float y)
	{
		m_owner->m_pos.x += x;
		m_owner->m_pos.y += y;
	}

}
}
