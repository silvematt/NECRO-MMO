#include "Cmd.h"
#include "NECROEngine.h"
#include "Collider.h"
#include "Player.h"

#include "SDL.h"

namespace NECRO
{
namespace Client
{
	//----------------------------------------------------------------------------------------------
	// Executes the command bound to the routine function pointer
	//----------------------------------------------------------------------------------------------
	int Cmd::Execute(const std::vector<std::string>& args)
	{
		if (routine)
			return (this->*routine)(args);

		return 1; // 1 means problem
	}

	//----------------------------------------------------------------------------------------------
	// Logs help to the console
	//----------------------------------------------------------------------------------------------
	int Cmd::Cmd_Help(const std::vector<std::string>& args)
	{
		Console& c = engine.GetConsole();

		c.Log("'teleport' (x, y): teleports the player to the x,y grid coordinates.");
		c.Log("'noclip' (): toggles collision detection for the player.");
		c.Log("'dcoll' (): toggles collision debug for all entities.");
		c.Log("'doccl' (): toggles occlusion debug for entities that can occlude the player.");
		c.Log("'qqq' (): quits the game.");
		c.Log("'authconnect' (): connects to the auth server.");

		return 1; // return 1 to not close the console after this function
	}

	//----------------------------------------------------------------------------------------------
	// Teleports the player to a grid pos - TODO: make this cmd take an additional entity param
	//----------------------------------------------------------------------------------------------
	int Cmd::Cmd_TeleportToGrid(const std::vector<std::string>& args)
	{
		Console& c = engine.GetConsole();

		if (args.size() < 3)
		{
			c.Log("CMD_TeleportToGrid: requires 2 arguments [x, y], but only " + std::to_string(args.size() - 1) + " were passed.");
			return 1;
		}
		else
		{
			int x = ClientUtility::TryParseInt(args[1]);
			int y = ClientUtility::TryParseInt(args[2]);

			// Teleport player
			engine.GetGame().GetCurPlayer()->TeleportToGrid(x, y);

			return 0;
		}
	}

	//----------------------------------------------------------------------------------------------
	// Disables collision checking for the player
	//----------------------------------------------------------------------------------------------
	int Cmd::Cmd_NoClip(const std::vector<std::string>& args)
	{
		Console& c = engine.GetConsole();
		Collider* pColl = engine.GetGame().GetCurPlayer()->GetCollider();

		pColl->m_enabled = !pColl->m_enabled;

		std::string s = "No Clip: ";
		s.append((!pColl->m_enabled ? "enabled" : "disabled"));
		c.Log(s);

		return 0;
	}

	//----------------------------------------------------------------------------------------------
	// Toggles the universal collision debug for entiteis
	//----------------------------------------------------------------------------------------------
	int Cmd::Cmd_ToggleCollisionDebug(const std::vector<std::string>& args)
	{
		Console& c = engine.GetConsole();

		// If layer is provided, always enable
		if (args.size() >= 2)
		{
			int layerVal = ClientUtility::TryParseInt(args[1]);
			Entity::DEBUG_COLLIDER_LAYER = layerVal;
			Entity::DEBUG_COLLIDER_ENABLED = true;
		}
		else // otherwise toggle
			Entity::DEBUG_COLLIDER_ENABLED = !Entity::DEBUG_COLLIDER_ENABLED;

		if (Entity::DEBUG_COLLIDER_ENABLED)
			c.Log("Debug Collision: enabled.");
		else
			c.Log("Debug Collision: disabled.");

		return 0;
	}

	//----------------------------------------------------------------------------------------------
	// Toggles the universal occlusion debug for entiteis
	//----------------------------------------------------------------------------------------------
	int Cmd::Cmd_ToggleOcclusionDebug(const std::vector<std::string>& args)
	{
		Console& c = engine.GetConsole();

		Entity::DEBUG_OCCLUSION_ENABLED = !Entity::DEBUG_OCCLUSION_ENABLED;

		if (Entity::DEBUG_OCCLUSION_ENABLED)
			c.Log("Debug Occlusion: enabled.");
		else
			c.Log("Debug Occlusion: disabled.");

		return 0;
	}

	//----------------------------------------------------------------------------------------------
	// Quits the game
	//----------------------------------------------------------------------------------------------
	int Cmd::Cmd_QuitApplication(const std::vector<std::string>& args)
	{
		engine.Stop();

		return 0;
	}


	int Cmd::Cmd_ConnectToAuthServer(const std::vector<std::string>& args)
	{
		Console& c = engine.GetConsole();

		if (args.size() < 3)
		{
			c.Log("CMD_ConnectToAuthServer: requires 2 arguments [username, password] .");
			return 1;
		}

		c.Log("Attempting to connect as : '" + args[1] + "'...");

		engine.GetAuthManager().SetAuthDataUsername(args[1]);
		engine.GetAuthManager().SetAuthDataPassword(args[2]);

		engine.GetAuthManager().ConnectToAuthServer();

		return 0;
	}

}
}
