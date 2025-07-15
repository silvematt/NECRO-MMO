#ifndef NECROPLAYER_H
#define NECROPLAYER_H

#include "Entity.h"
#include "Game.h"
#include "Animator.h"
#include "Collider.h"

namespace NECRO
{
namespace Client
{
	inline constexpr float PLAYER_MOVE_SPEED_FREE = 180.0f;
	inline constexpr float PLAYER_MOVE_SPEED_AIM = 100.0f;

	// Dimensions of each player frame
	inline constexpr int PLAYER_WIDTH = 128;
	inline constexpr int PLAYER_HEIGHT = 128;

	inline constexpr int HALF_PLAYER_WIDTH = 64;
	inline constexpr int HALF_PLAYER_HEIGHT = 64;

	inline constexpr float PLAYER_CONST_Z_POS = 0.01f; // a constant added to the player's zPos when modified

	//-------------------------------------------------
	// Player class, derived by Entity
	//-------------------------------------------------
	class Player : public Entity
	{
	private:
		float			m_curMoveSpeed = 2.5f;
		IsoDirection	m_isoDirection = IsoDirection::SOUTH;		// The isometric direction the player is facing

		float m_deltaX = 0.0f;
		float m_deltaY = 0.0f;

		bool m_wasAiming = false;
		bool m_isAiming = false;									// Is the player in aim mode?
		bool m_wasMoving = false;
		bool m_isMoving = false;

		// Relative mouse pos used when aiming
		float m_relativeMouseX;
		float m_relativeMouseY;

		// List of close (8-neighbours close) entities, filled every frame
		std::vector<Entity*> m_closeEntities;

	private:
		void			CalculateIsoDirection(float deltaX, float deltaY);
		void			CalculateIsoDirectionWhileAiming();
		void			HandleMovements();
		void			HandleAnim();

		void			UpdateCloseEntities();

	public:
		~Player();

		static uint32_t	ENT_ID;
		bool			m_controlsEnabled = true; // TEST: 

	public:
		void			Init();
		void			Update() override;

		float			GetCurMoveSpeed() const;

		void			SetControlsEnabled(bool e);

		void			TeleportToGrid(int x, int y);

		void			OnCellChanges() override;
	};

	inline float Player::GetCurMoveSpeed() const
	{
		return m_curMoveSpeed;
	}

	inline void Player::SetControlsEnabled(bool e)
	{
		m_controlsEnabled = e;
	}

}
}

#endif
