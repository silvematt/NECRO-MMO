#include "Player.h"

#include "NECROEngine.h"
#include "Game.h"

namespace NECRO
{
namespace Client
{
	// Initialize static member
	uint32_t Player::ENT_ID = 0;

	Player::~Player()
	{
		// Set cur player to nullptr in Game when destroyed
		engine.GetGame().SetCurPlayer(nullptr);
	}

	//-------------------------------------------------
	// Initialize the player
	//-------------------------------------------------
	void Player::Init()
	{
		// Construct Animator
		CreateAnimator();
		m_anim->Init(this);
		Animator* animAsset = engine.GetAssetsManager().GetAnimator("player_war.nanim");
		if (animAsset)
			*(m_anim.get()) = *engine.GetAssetsManager().GetAnimator("player_war.nanim");
		m_anim->Play("idle"); // Set default

		// Construct Collider
		CreateCollider();
		m_coll->Init(true, this, 0, 0, 32, 16);
		m_coll->SetOffset(0, -16);

		// Add player light
		/*
		CreateLight();
		Light* thisLight = GetLight();

		thisLight->color.r = 4;
		thisLight->color.g = 4;
		thisLight->color.b = 4;
		thisLight->intensity = 1;
		thisLight->radius = 2;
		thisLight->pos.x = 0;
		thisLight->pos.y = 0;
		*/

		// TODO: We can prefab players as well at least for basic info like occlModifier if we're going to have more data, we will probably have different characters with maybe different sizes
		m_occlModifierX = 100;
		m_occlModifierY = 75;
	}

	//-------------------------------------------------
	// Updates the Player
	//-------------------------------------------------
	void Player::Update()
	{
		UpdateCloseEntities();
		HandleMovements();
		HandleAnim();

		// Update the entity base, pos, gridPos, isoPos etc.
		Entity::Update();
	}

	void Player::HandleMovements()
	{
		m_wasMoving = m_isMoving;
		m_wasAiming = m_isAiming;

		Input& input = engine.GetInput();

		m_deltaX = 0.0f;
		m_deltaY = 0.0f;

		SetControlsEnabled(!engine.GetConsole().IsOpen());
		if (m_controlsEnabled)
		{
			// Get input
			if (input.GetKeyHeld(SDL_SCANCODE_W))
				m_deltaX -= 1;
			if (input.GetKeyHeld(SDL_SCANCODE_S))
				m_deltaX += 1;
			if (input.GetKeyHeld(SDL_SCANCODE_D))
				m_deltaY -= 1;
			if (input.GetKeyHeld(SDL_SCANCODE_A))
				m_deltaY += 1;

			// Go 1 layer up or down
			if (input.GetKeyDown(SDL_SCANCODE_KP_MINUS))
				m_zPos -= 100;
			if (input.GetKeyDown(SDL_SCANCODE_KP_PLUS))
				m_zPos += 100;

			//SDL_Log("%f", zPos);

			m_isMoving = (m_deltaX != 0.0f || m_deltaY != 0.0f) ? true : false;
			m_isAiming = input.GetMouseHeld(static_cast<SDL_Scancode>(SDL_BUTTON_RIGHT));
		}
		else
		{
			m_deltaX = 0;
			m_deltaY = 0;

			m_isMoving = false;
			m_isAiming = false;
		}

		// Calculate direction
		if (!m_isAiming)
			CalculateIsoDirection(m_deltaX, m_deltaY);
		else
			CalculateIsoDirectionWhileAiming();

		// Select tile from tileset (used for rendering)
		m_tilesetYOff = static_cast<int>(m_isoDirection);		// pick y offset in base of direction

		// Build raw input vector
		Vector2 cartMove(m_deltaX, m_deltaY);

		// Normalize to prevent faster diagonal movement
		if (cartMove.x != 0 || cartMove.y != 0)
			cartMove.Normalize();

		// Convert to isometric directions
		Vector2 isoMove;
		isoMove.x = (cartMove.x - cartMove.y) * 0.5f;
		isoMove.y = (cartMove.x + cartMove.y) * 0.25f;

		// Set speed
		m_curMoveSpeed = m_isAiming ? PLAYER_MOVE_SPEED_AIM : PLAYER_MOVE_SPEED_FREE;

		// Apply movement
		float moveAmountX = isoMove.x * m_curMoveSpeed * engine.GetDeltaTime();
		float moveAmountY = isoMove.y * m_curMoveSpeed * engine.GetDeltaTime();

		// Checking is performed on each axis to allow the player to slide on the colliders
		m_pos.x += moveAmountX;
		m_coll->Update();

		for (auto const& e : m_closeEntities)
		{
			if (e->GetLayer() == GetLayerFromZPos()) // Check if the collision is happening on the same layer
				if (e->HasCollider())
					if (m_coll->TestIntersection(e->GetCollider()))
					{
						m_pos.x -= moveAmountX;
						break;
					}
		}

		m_pos.y += moveAmountY;
		m_coll->Update();

		for (auto const& e : m_closeEntities)
		{
			if (e->GetLayer() == GetLayerFromZPos()) // Check if the collision is happening on the same layer
				if (e->HasCollider())
					if (m_coll->TestIntersection(e->GetCollider()))
					{
						m_pos.y -= moveAmountY;
						break;
					}
		}

		m_coll->Update();
	}


	//---------------------------------------------------------------------------------------
	// Updates the closeEntities vector
	//---------------------------------------------------------------------------------------
	void Player::UpdateCloseEntities()
	{
		m_closeEntities.clear();

		for (int x = -1; x <= 1; x++)
			for (int y = -1; y <= 1; y++)
			{
				Cell* c = m_owner->GetWorld()->GetCellAt(m_gridPosX + x, m_gridPosY + y);

				if (c)
				{
					for (int i = 0; i < c->GetEntitiesPtrSize(); i++)
						if (c->GetEntityPtrAt(i)->GetID() != this->m_ID)
							m_closeEntities.push_back(c->GetEntityPtrAt(i));
				}
			}
	}

	//---------------------------------------------------------------------------------------
	// Handles the selection of animations for player's movements (free/aim mode)
	//---------------------------------------------------------------------------------------
	void Player::HandleAnim()
	{
		// If is in aim mode
		if (m_isAiming)
		{
			if (!m_isMoving)
			{
				// If the player is not moving, just play the standing anim
				if (m_anim->GetCurStateNamePlaying() != "aim_stand")
					m_anim->Play("aim_stand");
			}
			else
			{
				// Strafe or walk check, in base of the direction and input, compute if the player should walk or strafe
				if (m_isoDirection == IsoDirection::NORTH || m_isoDirection == IsoDirection::SOUTH)
				{
					// Check strafe
					if (m_deltaY != 0.0f)
					{
						if (m_anim->GetCurStateNamePlaying() != "aim_strafe")
							m_anim->Play("aim_strafe");
					}
					else
						if (m_anim->GetCurStateNamePlaying() != "aim_walk")
							m_anim->Play("aim_walk");
				}
				else if (m_isoDirection == IsoDirection::NORTH_EAST || m_isoDirection == IsoDirection::SOUTH_WEST)
				{
					// Check strafe
					if (fabs(m_deltaY - m_deltaX) != 0.0f)
					{
						if (m_anim->GetCurStateNamePlaying() != "aim_strafe")
							m_anim->Play("aim_strafe");
					}
					else
						if (m_anim->GetCurStateNamePlaying() != "aim_walk")
							m_anim->Play("aim_walk");
				}
				else if (m_isoDirection == IsoDirection::EAST || m_isoDirection == IsoDirection::WEST)
				{
					// Check strafe
					if (m_deltaX != 0.0f)
					{
						if (m_anim->GetCurStateNamePlaying() != "aim_strafe")
							m_anim->Play("aim_strafe");
					}
					else
						if (m_anim->GetCurStateNamePlaying() != "aim_walk")
							m_anim->Play("aim_walk");
				}
				else if (m_isoDirection == IsoDirection::NORTH_WEST || m_isoDirection == IsoDirection::SOUTH_EAST)
				{
					if (fabs(m_deltaY - m_deltaX) < 2.0f)
					{
						if (m_anim->GetCurStateNamePlaying() != "aim_strafe")
							m_anim->Play("aim_strafe");
					}
					else
						if (m_anim->GetCurStateNamePlaying() != "aim_walk")
							m_anim->Play("aim_walk");
				}
			}
		}
		else
		{
			if (m_isMoving)
			{
				if (m_anim->GetCurStateNamePlaying() != "run")
					m_anim->Play("run");
			}
			else
			{
				if (m_anim->GetCurStateNamePlaying() != "idle")
					m_anim->Play("idle");
			}
		}

		m_anim->Update();
	}

	//-------------------------------------------------
	// Given the input, set isoDirection
	//-------------------------------------------------
	void Player::CalculateIsoDirection(float deltaX, float deltaY)
	{
		// Determine the direction of the player in base of the input
		// TODO: may have to delay this by few ms because at high framerates it's hard to keep an oblique direction
		if (deltaX < 0.0f)
		{
			if (deltaY < 0.0f)
				m_isoDirection = IsoDirection::NORTH_EAST;
			else if (deltaY > 0.0f)
				m_isoDirection = IsoDirection::NORTH_WEST;
			else
				m_isoDirection = IsoDirection::NORTH;
		}
		else if (deltaX > 0.0f)
		{
			if (deltaY < 0.0f)
				m_isoDirection = IsoDirection::SOUTH_EAST;
			else if (deltaY > 0.0f)
				m_isoDirection = IsoDirection::SOUTH_WEST;
			else
				m_isoDirection = IsoDirection::SOUTH;
		}
		else if (deltaY < 0.0f)
		{
			if (deltaX == 0.0f)
				m_isoDirection = IsoDirection::EAST;
		}
		else if (deltaY > 0.0f)
		{
			if (deltaX == 0.0f)
				m_isoDirection = IsoDirection::WEST;
		}
	}

	//-------------------------------------------------
	// Set isoDirection in aiming mode
	//-------------------------------------------------
	void Player::CalculateIsoDirectionWhileAiming()
	{
		Input& input = engine.GetInput();

		Camera* curCamera = engine.GetGame().GetMainCamera();
		float curZoom = curCamera->GetZoom();

		// Get relative mouse pos 
		Vector2 playerScreenPos;
		playerScreenPos.x = m_isoPos.x + (m_img->GetTilesetWidth() / 2);
		playerScreenPos.y = m_isoPos.y + (m_img->GetTilesetHeight() / 2);

		m_relativeMouseX = playerScreenPos.x - (input.GetMouseX() / curZoom);
		m_relativeMouseY = playerScreenPos.y - (input.GetMouseY() / curZoom);

		// Calculate the angle (while accounting for zoom)
		float angle = atan2f(m_relativeMouseY, m_relativeMouseX);

		// Use degrees, wrap around negatives
		angle = angle * (180.0 / M_PI);
		if (angle < 0)
			angle += 360.0;

		angle = fmod(angle, 360.0f);

		// Determine direction based on angle
		if ((angle >= 337.5f && angle <= 360.0f) || (angle >= 0.0f && angle < 22.5f))
			m_isoDirection = IsoDirection::WEST;
		else if (angle >= 22.5f && angle < 67.5f)
			m_isoDirection = IsoDirection::NORTH_WEST;
		else if (angle >= 67.5f && angle < 112.5f)
			m_isoDirection = IsoDirection::NORTH;
		else if (angle >= 112.5f && angle < 157.5f)
			m_isoDirection = IsoDirection::NORTH_EAST;
		else if (angle >= 157.5f && angle < 202.5f)
			m_isoDirection = IsoDirection::EAST;
		else if (angle >= 202.5f && angle < 247.5f)
			m_isoDirection = IsoDirection::SOUTH_EAST;
		else if (angle >= 247.5f && angle < 292.5f)
			m_isoDirection = IsoDirection::SOUTH;
		else
			m_isoDirection = IsoDirection::SOUTH_WEST;
	}

	//-------------------------------------------------
	// Teleports immediatly to location
	//-------------------------------------------------
	void Player::TeleportToGrid(int x, int y)
	{
		if (m_owner->GetWorld()->IsInWorldBounds(x, y))
		{
			m_pos.x = CELL_WIDTH * x;
			m_pos.y = CELL_HEIGHT * y;

			TransferToCellImmediately(m_owner->GetWorld()->GetCellAt(x, y));
		}
	}

	void Player::OnCellChanges()
	{
		// Sets the Z pos of the player equal to the current cell if there's a ZModifier, used to go up stairs and come down from them
		float zMod = m_owner->GetZModifier();
		if (zMod > 0.0)
			m_zPos = zMod + PLAYER_CONST_Z_POS;
	}

}
}
