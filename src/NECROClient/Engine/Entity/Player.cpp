#include "Player.h"

#include "NECROClient.h"
#include "Game.h"

// Initialize static member
uint32_t Player::ENT_ID = 0;

Player::~Player()
{
	// Set cur player to nullptr in NECROGame when destroyed
	engine.GetGame().SetCurPlayer(nullptr);
}

//-------------------------------------------------
// Initialize the player
//-------------------------------------------------
void Player::Init()
{
	// Construct Animator
	CreateAnimator();
	anim->Init(this);
	Animator* animAsset = engine.GetAssetsManager().GetAnimator("player_war.nanim");
	if(animAsset)
		*(anim.get()) = *engine.GetAssetsManager().GetAnimator("player_war.nanim");
	anim->Play("idle"); // Set default

	// Construct Collider
	CreateCollider();
	coll->Init(true, this, 0, 0, 32, 16);
	coll->SetOffset(0, -16);

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
	occlModifierX = 100;
	occlModifierY = 75;
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
	wasMoving = isMoving;
	wasAiming = isAiming;

	NECROInput& input = engine.GetInput();

	deltaX = 0.0f; 
	deltaY = 0.0f;

	SetControlsEnabled(!engine.GetConsole().IsOpen());
	if (controlsEnabled)
	{
		// Get input
		if (input.GetKeyHeld(SDL_SCANCODE_W))
			deltaX -= 1;
		if (input.GetKeyHeld(SDL_SCANCODE_S))
			deltaX += 1;
		if (input.GetKeyHeld(SDL_SCANCODE_D))
			deltaY -= 1;
		if (input.GetKeyHeld(SDL_SCANCODE_A))
			deltaY += 1;

		// Go 1 layer up or down
		if (input.GetKeyDown(SDL_SCANCODE_KP_MINUS))
			zPos -= 100;
		if (input.GetKeyDown(SDL_SCANCODE_KP_PLUS))
			zPos += 100;

		 //SDL_Log("%f", zPos);

		isMoving = (deltaX != 0.0f || deltaY != 0.0f) ? true : false;
		isAiming = input.GetMouseHeld(static_cast<SDL_Scancode>(SDL_BUTTON_RIGHT));
	}
	else
	{
		deltaX = 0;
		deltaY = 0;

		isMoving = false;
		isAiming = false;
	}

	// Calculate direction
	if (!isAiming)
		CalculateIsoDirection(deltaX, deltaY);
	else
		CalculateIsoDirectionWhileAiming();

	// Select tile from tileset (used for rendering)
	tilesetYOff = isoDirection;		// pick y offset in base of direction

	/*
	* WASD NON-ISOMETRIC ALLIGNED MOVEMENT
	// Normalize the input
	Vector2 moveInput(deltaX, deltaY);
	moveInput.Normalize();

	// Convert to Isometric
	// divison CELL_ components are inverted to equalize the movement
	NMath::CartToIso(moveInput.x / CELL_WIDTH, moveInput.y / CELL_HEIGHT, moveInput.x, moveInput.y);

	// Set speed
	curMoveSpeed = isAiming ? PLAYER_MOVE_SPEED_AIM : PLAYER_MOVE_SPEED_FREE;

	// Move while accounting for close entities colliders
	float moveAmountX = moveInput.x * curMoveSpeed * engine.GetDeltaTime();
	float moveAmountY = moveInput.y * curMoveSpeed * engine.GetDeltaTime();
	*/

	// Build raw input vector
	Vector2 cartMove(deltaX, deltaY);

	// Normalize to prevent faster diagonal movement
	if (cartMove.x != 0 || cartMove.y != 0)
		cartMove.Normalize();

	// Convert to isometric directions
	Vector2 isoMove;
	isoMove.x = (cartMove.x - cartMove.y) * 0.5f;
	isoMove.y = (cartMove.x + cartMove.y) * 0.25f;

	// Set speed
	curMoveSpeed = isAiming ? PLAYER_MOVE_SPEED_AIM : PLAYER_MOVE_SPEED_FREE;

	// Apply movement
	float moveAmountX = isoMove.x * curMoveSpeed * engine.GetDeltaTime();
	float moveAmountY = isoMove.y * curMoveSpeed * engine.GetDeltaTime();

	// Checking is performed on each axis to allow the player to slide on the colliders
	pos.x += moveAmountX;
	coll->Update();

	for (auto const& e : closeEntities)
	{
		if(e->GetLayer() == GetLayerFromZPos()) // Check if the collision is happening on the same layer
			if (e->HasCollider())
				if (coll->TestIntersection(e->GetCollider()))
				{
					pos.x -= moveAmountX;
					break;
				}
	}

	pos.y += moveAmountY;
	coll->Update();

	for (auto const& e : closeEntities)
	{
		if (e->GetLayer() == GetLayerFromZPos()) // Check if the collision is happening on the same layer
			if(e->HasCollider())
				if (coll->TestIntersection(e->GetCollider()))
				{
					pos.y -= moveAmountY;
					break;
				}
	}

	coll->Update();
}


//---------------------------------------------------------------------------------------
// Updates the closeEntities vector
//---------------------------------------------------------------------------------------
void Player::UpdateCloseEntities()
{
	closeEntities.clear();

	for (int x = -1; x <= 1; x++)
		for (int y = -1; y <= 1; y++)
		{
			Cell* c = owner->GetWorld()->GetCellAt(gridPosX + x, gridPosY + y);

			if (c)
			{
				for (int i = 0; i < c->GetEntitiesPtrSize(); i++)
					if(c->GetEntityPtrAt(i)->GetID() != this->ID)
						closeEntities.push_back(c->GetEntityPtrAt(i));
			}
		}
}

//---------------------------------------------------------------------------------------
// Handles the selection of animations for player's movements (free/aim mode)
//---------------------------------------------------------------------------------------
void Player::HandleAnim()
{
	// If is in aim mode
	if (isAiming)
	{
		if (!isMoving)
		{
			// If the player is not moving, just play the standing anim
			if (anim->GetCurStateNamePlaying() != "aim_stand")
				anim->Play("aim_stand");
		}
		else
		{
			 // Strafe or walk check, in base of the direction and input, compute if the player should walk or strafe
			if (isoDirection == IsoDirection::NORTH || isoDirection == IsoDirection::SOUTH)
			{
				// Check strafe
				if (deltaY != 0.0f)
				{
					if (anim->GetCurStateNamePlaying() != "aim_strafe")
						anim->Play("aim_strafe");
				}
				else
					if (anim->GetCurStateNamePlaying() != "aim_walk")
						anim->Play("aim_walk");
			}
			else if (isoDirection == IsoDirection::NORTH_EAST || isoDirection == IsoDirection::SOUTH_WEST)
			{
				// Check strafe
				if (fabs(deltaY - deltaX) != 0.0f)
				{
					if (anim->GetCurStateNamePlaying() != "aim_strafe")
						anim->Play("aim_strafe");
				}
				else
					if (anim->GetCurStateNamePlaying() != "aim_walk")
						anim->Play("aim_walk");
			}
			else if (isoDirection == IsoDirection::EAST || isoDirection == IsoDirection::WEST)
			{
				// Check strafe
				if (deltaX != 0.0f)
				{
					if (anim->GetCurStateNamePlaying() != "aim_strafe")
						anim->Play("aim_strafe");
				}
				else
					if (anim->GetCurStateNamePlaying() != "aim_walk")
						anim->Play("aim_walk");
			}
			else if (isoDirection == IsoDirection::NORTH_WEST || isoDirection == IsoDirection::SOUTH_EAST)
			{
				if (fabs(deltaY - deltaX) < 2.0f)
				{
					if (anim->GetCurStateNamePlaying() != "aim_strafe")
						anim->Play("aim_strafe");
				}
				else
					if (anim->GetCurStateNamePlaying() != "aim_walk")
						anim->Play("aim_walk");
			}
		}
	}
	else
	{
		if (isMoving)
		{
			if (anim->GetCurStateNamePlaying() != "run")
				anim->Play("run");
		}
		else
		{
			if (anim->GetCurStateNamePlaying() != "idle")
				anim->Play("idle");
		}
	}

	anim->Update();
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
			isoDirection = IsoDirection::NORTH_EAST;
		else if (deltaY > 0.0f)
			isoDirection = IsoDirection::NORTH_WEST;
		else
			isoDirection = IsoDirection::NORTH;
	}
	else if (deltaX > 0.0f)
	{
		if (deltaY < 0.0f)
			isoDirection = IsoDirection::SOUTH_EAST;
		else if (deltaY > 0.0f)
			isoDirection = IsoDirection::SOUTH_WEST;
		else
			isoDirection = IsoDirection::SOUTH;
	}
	else if (deltaY < 0.0f)
	{
		if (deltaX == 0.0f)
			isoDirection = IsoDirection::EAST;
	}
	else if (deltaY > 0.0f)
	{
		if (deltaX == 0.0f)
			isoDirection = IsoDirection::WEST;
	}
}

//-------------------------------------------------
// Set isoDirection in aiming mode
//-------------------------------------------------
void Player::CalculateIsoDirectionWhileAiming()
{
	NECROInput& input = engine.GetInput();

	Camera* curCamera = engine.GetGame().GetMainCamera();
	float curZoom = curCamera->GetZoom();

	// Get relative mouse pos 
	Vector2 playerScreenPos;
	playerScreenPos.x = isoPos.x + (img->GetTilesetWidth() / 2);
	playerScreenPos.y = isoPos.y + (img->GetTilesetHeight() / 2);

	relativeMouseX = playerScreenPos.x - (input.GetMouseX() / curZoom);
	relativeMouseY = playerScreenPos.y - (input.GetMouseY() / curZoom);

	// Calculate the angle (while accounting for zoom)
	float angle = atan2f(relativeMouseY, relativeMouseX);

	// Use degrees, wrap around negatives
	angle = angle * (180.0 / M_PI);
	if (angle < 0)
		angle += 360.0;

	angle = fmod(angle, 360.0f);

	// Determine direction based on angle
	if ((angle >= 337.5f && angle <= 360.0f) || (angle >= 0.0f && angle < 22.5f))
		isoDirection = IsoDirection::WEST;
	else if (angle >= 22.5f && angle < 67.5f)
		isoDirection = IsoDirection::NORTH_WEST;
	else if (angle >= 67.5f && angle < 112.5f)
		isoDirection = IsoDirection::NORTH;
	else if (angle >= 112.5f && angle < 157.5f)
		isoDirection = IsoDirection::NORTH_EAST;
	else if (angle >= 157.5f && angle < 202.5f)
		isoDirection = IsoDirection::EAST;
	else if (angle >= 202.5f && angle < 247.5f)
		isoDirection = IsoDirection::SOUTH_EAST;
	else if (angle >= 247.5f && angle < 292.5f)
		isoDirection = IsoDirection::SOUTH;
	else
		isoDirection = IsoDirection::SOUTH_WEST;
}

//-------------------------------------------------
// Teleports immediatly to location
//-------------------------------------------------
void Player::TeleportToGrid(int x, int y)
{
	if (owner->GetWorld()->IsInWorldBounds(x, y))
	{
		pos.x = CELL_WIDTH * x;
		pos.y = CELL_HEIGHT * y;

		TransferToCellImmediately(owner->GetWorld()->GetCellAt(x, y));
	}
}

void Player::OnCellChanges()
{
	// Sets the Z pos of the player equal to the current cell if there's a ZModifier, used to go up stairs and come down from them
	float zMod = owner->GetZModifier();
	if (zMod > 0.0)
		zPos = zMod + PLAYER_CONST_Z_POS;
}
