#ifndef NECROPLAYER_H
#define NECROPLAYER_H

#include "Entity.h"
#include "Game.h"
#include "Animator.h"
#include "Collider.h"
#include "WorldCodes.h"

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

	// Vertical offset where the nameplate is drawn
	inline constexpr int PLAYER_NAMEPLATE_Y_OFFSET = 10;

	//-------------------------------------------------
	// Player class, derived by Entity
	//-------------------------------------------------
	class Player : public Entity
	{
	private:
		float			m_curMoveSpeed = 2.5f;

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

		// Name drawn on top of the player's head, in the form <Name>. If it's empty no text will be drawn
		std::string m_displayName;

		// Remote players are the other players spawned by the server, they don't read input and don't have a collider
		bool m_isRemote = false;

	private:
		void			CalculateIsoDirection(float deltaX, float deltaY);
		void			CalculateIsoDirectionWhileAiming();
		void			HandleMovements();
		void			HandleAnim();

		void			UpdateCloseEntities();

	public:
		explicit Player(bool isRemote = false) : m_isRemote(isRemote) {}
		~Player();

		static uint32_t	ENT_ID;
		static Player*	ENT_PTR;

		bool			m_controlsEnabled = true; // TEST: 

		// True if the movement changed since the last SendPlayerMovementUpdate
		bool			m_isMovementDirty = false;
		float			m_previousPosX = 0.0f;
		float			m_previousPosY = 0.0f;
		float			m_previousPosZ = 0.0f;
		IsoDirection	m_previousDirection = IsoDirection::NORTH;

	public:
		void			Init();
		void			Update() override;
		void			Draw() override;

		void			SetName(const std::string& name);

		float			GetCurMoveSpeed() const;

		void			SetControlsEnabled(bool e);

		void			TeleportToGrid(int x, int y);

		void			OnCellChanges() override;

		void			ExecuteMovementCorrection(const NECRO::World::CPacketPlayerMovementCorrection* correction);
	};

	inline float Player::GetCurMoveSpeed() const
	{
		return m_curMoveSpeed;
	}

	inline void Player::SetControlsEnabled(bool e)
	{
		m_controlsEnabled = e;
	}

	inline void Player::SetName(const std::string& name)
	{
		m_displayName = name.empty() ? "" : "<" + name + ">";
	}

}
}

#endif
