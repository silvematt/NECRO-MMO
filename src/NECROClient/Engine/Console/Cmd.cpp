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

		c.Log("'/teleport' (x, y): teleports the player to the x,y grid coordinates.");
		c.Log("'/noclip' (): toggles collision detection for the player.");
		c.Log("'/dcoll' (): toggles collision debug for all entities.");
		c.Log("'/doccl' (): toggles occlusion debug for entities that can occlude the player.");
		c.Log("'/qqq' (): quits the game.");
		c.Log("'/authconnect' (): connects to the auth server.");
		c.Log("'/authworld' (ip, port): connects to a world server (run after 'authconnect').");
		c.Log("'/createchar' (name, race, class, gender): creates a new character.");
		c.Log("'/deletechar' (charID, charName): deletes one of your characters.");
		c.Log("'/enumchars' (): enums all the characters on this account.");
		c.Log("'/enterworld' (charID): sends a request to the server to enter in the world with the poassed character ID.");

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

	//----------------------------------------------------------------------------------------------
	// Connects to the AuthServer
	//----------------------------------------------------------------------------------------------

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

	//----------------------------------------------------------------------------------------------
	// Connects to a World Server at the given <ip> <port>
	//----------------------------------------------------------------------------------------------
	int Cmd::Cmd_ConnectToWorldServer(const std::vector<std::string>& args)
	{
		Console& c = engine.GetConsole();

		if (args.size() < 3)
		{
			c.Log("CMD_ConnectToWorldServer: requires 2 arguments [ip, port].");
			return 1;
		}

		if (!engine.GetAuthManager().GetData().hasAuthenticated)
		{
			c.Log("Not authenticated. Run 'authconnect [user, pass]' first.");
			return 1;
		}

		const std::string& ip = args[1];
		int parsedPort = ClientUtility::TryParseInt(args[2]);
		uint16_t port = static_cast<uint16_t>(parsedPort);

		c.Log("Attempting to connect to world server " + ip + ":" + std::to_string(port) + "...");

		engine.GetWorldManager().ConnectToWorldServer(ip, port);

		return 0;
	}

	int Cmd::Cmd_CreateCharacter(const std::vector<std::string>& args)
	{
		Console& c = engine.GetConsole();

		if (args.size() < 5)
		{
			c.Log("Cmd_CreateCharacter: requires 4 arguments [name, race, class, gender].");
			return 1;
		}

		if (!engine.GetWorldManager().GetData().isAuthed)
		{
			c.Log("You are not connected to the world server.");
			return 1;
		}

		const std::string& name = args[1];
		int race = ClientUtility::TryParseInt(args[2]);
		int charClass = ClientUtility::TryParseInt(args[3]);
		int gender = ClientUtility::TryParseInt(args[4]);

		Packet p;

		p << static_cast<uint16_t>(NECRO::World::PacketIDs::CHAR_CREATE_NEW);
		p << static_cast<uint16_t>((sizeof(NECRO::World::SPacketCreateNewChar)-1) - NECRO::World::S_PACKET_CREATE_NEW_CHAR_INITIAL_SIZE + name.length());

		p << static_cast<uint8_t>(race);
		p << static_cast<uint8_t>(charClass);
		p << static_cast<uint8_t>(gender);
		p << static_cast<uint8_t>(name.length());
		p << name;

		NetworkMessage encrypted(std::move(p));
		if (encrypted.AESEncrypt(engine.GetAuthManager().GetData().sessionKey.data(), engine.GetAuthManager().GetData().iv, nullptr, 0) < 0)
		{
			LOG_ERROR("Failed to encrypt packet.");
			c.Log("Failed to encrypt packet.");
			return 1;
		}
		NetworkMessage m(std::move(encrypted));
		engine.GetWorldManager().QueuePacket(NetworkMessage(std::move(m)));

		return 0;
	}

	int Cmd::Cmd_DeleteCharacter(const std::vector<std::string>& args)
	{
		Console& c = engine.GetConsole();

		if (args.size() < 3)
		{
			c.Log("Cmd_DeleteCharacter: requires 2 arguments [charID, charName].");
			return 1;
		}

		if (!engine.GetWorldManager().GetData().isAuthed)
		{
			c.Log("You are not connected to the world server.");
			return 1;
		}

		int characterID = ClientUtility::TryParseInt(args[1]);
		const std::string& characterName = args[2];

		Packet p;

		p << static_cast<uint16_t>(NECRO::World::PacketIDs::CHAR_DELETE_CHARACTER);
		p << static_cast<uint16_t>((sizeof(NECRO::World::SPacketDeleteCharacter)-1) - NECRO::World::S_PACKET_DELETE_CHAR_INITIAL_SIZE + characterName.length());
		p << static_cast<uint32_t>(characterID);
		p << static_cast<uint8_t>(characterName.length());
		p << characterName;

		NetworkMessage encrypted(std::move(p));
		if (encrypted.AESEncrypt(engine.GetAuthManager().GetData().sessionKey.data(), engine.GetAuthManager().GetData().iv, nullptr, 0) < 0)
		{
			LOG_ERROR("Failed to encrypt packet.");
			c.Log("Failed to encrypt packet.");
			return 1;
		}
		NetworkMessage m(std::move(encrypted));
		engine.GetWorldManager().QueuePacket(NetworkMessage(std::move(m)));

		return 0;
	}

	int Cmd::Cmd_EnumCharacters(const std::vector<std::string>& args)
	{
		Console& c = engine.GetConsole();

		if (!engine.GetWorldManager().GetData().isAuthed)
		{
			c.Log("You are not connected to the world server.");
			return 1;
		}

		Packet p;
		p << static_cast<uint16_t>(NECRO::World::PacketIDs::ENUM_CHARACTERS);

		NetworkMessage encrypted(std::move(p));
		if (encrypted.AESEncrypt(engine.GetAuthManager().GetData().sessionKey.data(), engine.GetAuthManager().GetData().iv, nullptr, 0) < 0)
		{
			LOG_ERROR("Failed to encrypt packet.");
			c.Log("Failed to encrypt packet.");
			return 1;
		}
		NetworkMessage m(std::move(encrypted));
		engine.GetWorldManager().QueuePacket(NetworkMessage(std::move(m)));

		return 0;
	}

	int Cmd::Cmd_EnterWorld(const std::vector<std::string>& args)
	{
		Console& c = engine.GetConsole();

		if (args.size() < 2)
		{
			c.Log("Cmd_EnterWorld: requires 1 arguments [charID].");
			return 1;
		}

		if (!engine.GetWorldManager().GetData().isAuthed)
		{
			c.Log("You are not connected to the world server.");
			return 1;
		}

		int characterID = ClientUtility::TryParseInt(args[1]);

		Packet p;

		p << static_cast<uint16_t>(NECRO::World::PacketIDs::ENTER_WORLD);
		p << static_cast<uint32_t>(characterID);

		NetworkMessage encrypted(std::move(p));
		if (encrypted.AESEncrypt(engine.GetAuthManager().GetData().sessionKey.data(), engine.GetAuthManager().GetData().iv, nullptr, 0) < 0)
		{
			LOG_ERROR("Failed to encrypt packet.");
			c.Log("Failed to encrypt packet.");
			return 1;
		}
		NetworkMessage m(std::move(encrypted));
		engine.GetWorldManager().QueuePacket(NetworkMessage(std::move(m)));

		return 0;
	}
}
}
