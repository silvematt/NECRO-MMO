#include "Console.h"
#include "NECROEngine.h"
#include "Cmd.h"

#include <algorithm>
#include <sstream>

namespace NECRO
{
namespace Client
{
	const char* CONSOLE_CMDS_LOG_FILENAME = "lastconsolecmdshistory.log";

	//-------------------------------------------------------
	// Initializes the console
	//-------------------------------------------------------
	int Console::Init()
	{
		inputField.Init(SDL_Rect{ 0,0,309,52 }, SDL_Rect{ 25, SCREEN_HEIGHT - 120, 380,35 }, "", engine.GetAssetsManager().GetImage("default_input_field.png"), engine.GetAssetsManager().GetImage("default_active_input_field.png"), 0);

		cmds.insert({ "help", Cmd(&Cmd::Cmd_Help) });
		cmds.insert({ "tel", Cmd(&Cmd::Cmd_TeleportToGrid) });
		cmds.insert({ "noclip", Cmd(&Cmd::Cmd_NoClip) });
		cmds.insert({ "dcoll", Cmd(&Cmd::Cmd_ToggleCollisionDebug) });
		cmds.insert({ "doccl", Cmd(&Cmd::Cmd_ToggleOcclusionDebug) });
		cmds.insert({ "qqq", Cmd(&Cmd::Cmd_QuitApplication) });
		cmds.insert({ "authconnect", Cmd(&Cmd::Cmd_ConnectToAuthServer) });

		// Load history if present
		cmdsLogFile.open(CONSOLE_CMDS_LOG_FILENAME, std::ios::in);

		// If we cannot open it, it's not there. It's not a error (may be first time the game is ran)
		if (!cmdsLogFile.is_open())
		{
			SDL_Log("CONSOLE: Could not open LogFile History: '%s'", CONSOLE_CMDS_LOG_FILENAME);
		}
		else
		{
			// Fill history
			std::string line;
			while (std::getline(cmdsLogFile, line))
			{
				if (!line.empty())
					history.push_back(std::move(line));
			}
		}

		cmdsLogFile.close();
		return 0;
	}

	//-------------------------------------------------------
	// Called when the engine is being shut down
	//-------------------------------------------------------
	int Console::Shutdown()
	{
		// On shutdown, write the last saved commands to lastconsolecmdshistory.log
		if (history.size() >= CONSOLE_CMD_HISTORY_MAX_LENGTH)
			cmdsLogFile.open(CONSOLE_CMDS_LOG_FILENAME, std::ios::out | std::ios::trunc);
		else
			cmdsLogFile.open(CONSOLE_CMDS_LOG_FILENAME, std::ios::out | std::ios::app);

		if (!cmdsLogFile.is_open())
		{
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "CONSOLE Error: Error occurred while trying to open LogFile History: '%s'", CONSOLE_CMDS_LOG_FILENAME);
			return 1;
		}

		// Write the commands to the file
		for (auto& cmd : history)
		{
			// Skip 'qqq'
			if (cmd == "qqq")
				continue;

			cmdsLogFile << cmd.c_str() << std::endl;
		}

		cmdsLogFile.close();
		return 0;
	}


	//-------------------------------------------------------
	// Toggles the console, handles send command as well
	//-------------------------------------------------------
	void Console::Toggle()
	{
		// If the console is active, see if we have to send cmd or close the console
		if (active)
		{
			if (inputField.str.size() > 0 && !ClientUtility::IsWhitespaceString(inputField.str))
			{
				// If the command is executed, close the console after sending it
				int val = SendCmd(inputField.str);
				if (val == 0)
					active = false;
			}
			else
				active = false;
		}
		else
		{
			active = true;
			curLineOffset = 0;
			curHistoryIndex = -1;
		}

		inputField.SetFocused(active);
	}


	//-------------------------------------------------------
	// Update routine
	//-------------------------------------------------------
	void Console::Update()
	{
		Input& i = engine.GetInput();

		if (i.GetKeyDown(SDL_SCANCODE_RETURN) || i.GetKeyDown(SDL_SCANCODE_KP_ENTER))
			Toggle();

		if (active)
		{
			if (i.GetKeyDown(SDL_SCANCODE_PAGEUP))
				curLineOffset += CONSOLE_MAX_LINES_PER_PAGE;
			else if (i.GetKeyDown(SDL_SCANCODE_PAGEDOWN))
				curLineOffset -= CONSOLE_MAX_LINES_PER_PAGE;

			curLineOffset = SDL_clamp(curLineOffset, 0, history.size());

			// Browse history with arrows
			if (i.GetKeyDown(SDL_SCANCODE_UP))
			{
				curHistoryIndex += 1;
				curHistoryIndex = SDL_clamp(curHistoryIndex, 0, history.size() - 1);

				int index = history.size() - 1 - curHistoryIndex;
				if (index >= 0 && index <= history.size() - 1)
					inputField.str = history[index];

			}
			else if (i.GetKeyDown(SDL_SCANCODE_DOWN))
			{
				curHistoryIndex -= 1;
				curHistoryIndex = SDL_clamp(curHistoryIndex, 0, history.size() - 1);

				int index = history.size() - 1 - curHistoryIndex;
				if (index >= 0 && index <= history.size() - 1)
					inputField.str = history[index];
			}

			Draw();
		}
	}

	//-------------------------------------------------------
	// Draws the console on the Overlay target
	//-------------------------------------------------------
	void Console::Draw()
	{
		engine.GetRenderer().SetRenderTarget(Renderer::DEBUG_TARGET);
		inputField.Draw();

		int count = 1;
		for (int i = logs.size() - 1; i >= 0; i--)
		{
			int actualOffset = 0;

			if (i - curLineOffset < logs.size())
				engine.GetRenderer().DrawTextDirectly(engine.GetAssetsManager().GetFont("defaultFont"), logs[i - curLineOffset].c_str(), 25, (SCREEN_HEIGHT - 120) - 40 * count, colorWhite);

			// Limit console
			if (count >= CONSOLE_MAX_LINES_PER_PAGE)
				break;

			count++;
		}
	}

	//-------------------------------------------------------
	// Adds the str to the console history
	//-------------------------------------------------------
	void Console::Log(const std::string& str)
	{
		// Check if logs vector is full, if it is, remove the first element before inserting another
		if (logs.size() >= CONSOLE_LOGS_MAX_LENGTH)
			logs.erase(logs.begin());

		logs.push_back(str);
	}

	//-------------------------------------------------------
	// Calls when a command is written and sent
	// 
	// RETURNS Cmd return value if the command is run
	// RETURNS 1 if the command was not found and just logged
	//-------------------------------------------------------
	int Console::SendCmd(const std::string& cmd)
	{
		// Check if history vector is full, if it is, remove the first element before inserting another
		if (history.size() >= CONSOLE_CMD_HISTORY_MAX_LENGTH)
			history.erase(history.begin());

		// Add command to history
		history.push_back(cmd);

		Log("> " + cmd); // log command

		// Get the input
		std::vector<std::string> input = SplitInput(cmd);

		// Process it
		if (input.size() > 0)
		{
			// Input[0] is the function name, transform it to lowercase
			std::transform(input[0].begin(), input[0].end(), input[0].begin(), std::tolower);

			auto it = cmds.find(input[0]);
			if (it != cmds.end())
			{
				// Command found, execute it
				return cmds.at(input[0]).Execute(input);
			} // ELSE command not registered, just keep it logged
		}

		return 1;
	}

	//-----------------------------------------------------------------------
	// Given a space-sperated string, splits it into a vector of strings
	//-----------------------------------------------------------------------
	std::vector<std::string> Console::SplitInput(const std::string& input)
	{
		std::vector<std::string> tokens;
		std::istringstream stream(input);
		std::string token;

		while (stream >> token)
			tokens.push_back(token);

		return tokens;
	}

}
}