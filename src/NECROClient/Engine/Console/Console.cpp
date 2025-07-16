#include "Console.h"
#include "NECROEngine.h"
#include "Cmd.h"

#include <algorithm>
#include <sstream>

namespace NECRO
{
namespace Client
{
	//-------------------------------------------------------
	// Initializes the console
	//-------------------------------------------------------
	int Console::Init()
	{
		m_inputField.Init(SDL_Rect{ 0,0,309,52 }, SDL_Rect{ 25, SCREEN_HEIGHT - 120, 380,35 }, "", engine.GetAssetsManager().GetImage("default_input_field.png"), engine.GetAssetsManager().GetImage("default_active_input_field.png"), 0);

		m_cmds.insert({ "help", Cmd(&Cmd::Cmd_Help) });
		m_cmds.insert({ "tel", Cmd(&Cmd::Cmd_TeleportToGrid) });
		m_cmds.insert({ "noclip", Cmd(&Cmd::Cmd_NoClip) });
		m_cmds.insert({ "dcoll", Cmd(&Cmd::Cmd_ToggleCollisionDebug) });
		m_cmds.insert({ "doccl", Cmd(&Cmd::Cmd_ToggleOcclusionDebug) });
		m_cmds.insert({ "qqq", Cmd(&Cmd::Cmd_QuitApplication) });
		m_cmds.insert({ "authconnect", Cmd(&Cmd::Cmd_ConnectToAuthServer) });

		// Load history if present
		m_cmdsLogFile.open(CONSOLE_CMDS_LOG_FILENAME, std::ios::in);

		// If we cannot open it, it's not there. It's not a error (may be first time the game is ran)
		if (!m_cmdsLogFile.is_open())
		{
			SDL_Log("CONSOLE: Could not open LogFile History: '%s'", CONSOLE_CMDS_LOG_FILENAME);
		}
		else
		{
			// Fill history
			std::string line;
			while (std::getline(m_cmdsLogFile, line))
			{
				if (!line.empty())
					m_history.push_back(std::move(line));
			}
		}

		m_cmdsLogFile.close();
		return 0;
	}

	//-------------------------------------------------------
	// Called when the engine is being shut down
	//-------------------------------------------------------
	int Console::Shutdown()
	{
		// On shutdown, write the last saved commands to lastconsolecmdshistory.log
		if (m_history.size() >= CONSOLE_CMD_HISTORY_MAX_LENGTH)
			m_cmdsLogFile.open(CONSOLE_CMDS_LOG_FILENAME, std::ios::out | std::ios::trunc);
		else
			m_cmdsLogFile.open(CONSOLE_CMDS_LOG_FILENAME, std::ios::out | std::ios::app);

		if (!m_cmdsLogFile.is_open())
		{
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "CONSOLE Error: Error occurred while trying to open LogFile History: '%s'", CONSOLE_CMDS_LOG_FILENAME);
			return 1;
		}

		// Write the commands to the file
		for (auto& cmd : m_history)
		{
			// Skip 'qqq'
			if (cmd == "qqq")
				continue;

			m_cmdsLogFile << cmd.c_str() << std::endl;
		}

		m_cmdsLogFile.close();
		return 0;
	}


	//-------------------------------------------------------
	// Toggles the console, handles send command as well
	//-------------------------------------------------------
	void Console::Toggle()
	{
		// If the console is active, see if we have to send cmd or close the console
		if (m_active)
		{
			if (m_inputField.m_str.size() > 0 && !ClientUtility::IsWhitespaceString(m_inputField.m_str))
			{
				// If the command is executed, close the console after sending it
				int val = SendCmd(m_inputField.m_str);
				if (val == 0)
					m_active = false;
			}
			else
				m_active = false;
		}
		else
		{
			m_active = true;
			m_curLineOffset = 0;
			m_curHistoryIndex = -1;
		}

		m_inputField.SetFocused(m_active);
	}


	//-------------------------------------------------------
	// Update routine
	//-------------------------------------------------------
	void Console::Update()
	{
		Input& i = engine.GetInput();

		if (i.GetKeyDown(SDL_SCANCODE_RETURN) || i.GetKeyDown(SDL_SCANCODE_KP_ENTER))
			Toggle();

		if (m_active)
		{
			if (i.GetKeyDown(SDL_SCANCODE_PAGEUP))
				m_curLineOffset += CONSOLE_MAX_LINES_PER_PAGE;
			else if (i.GetKeyDown(SDL_SCANCODE_PAGEDOWN))
				m_curLineOffset -= CONSOLE_MAX_LINES_PER_PAGE;

			m_curLineOffset = SDL_clamp(m_curLineOffset, 0, m_history.size());

			// Browse history with arrows
			if (i.GetKeyDown(SDL_SCANCODE_UP))
			{
				m_curHistoryIndex += 1;
				m_curHistoryIndex = SDL_clamp(m_curHistoryIndex, 0, m_history.size() - 1);

				int index = m_history.size() - 1 - m_curHistoryIndex;
				if (index >= 0 && index <= m_history.size() - 1)
					m_inputField.m_str = m_history[index];

			}
			else if (i.GetKeyDown(SDL_SCANCODE_DOWN))
			{
				m_curHistoryIndex -= 1;
				m_curHistoryIndex = SDL_clamp(m_curHistoryIndex, 0, m_history.size() - 1);

				int index = m_history.size() - 1 - m_curHistoryIndex;
				if (index >= 0 && index <= m_history.size() - 1)
					m_inputField.m_str = m_history[index];
			}

			Draw();
		}
	}

	//-------------------------------------------------------
	// Draws the console on the Overlay target
	//-------------------------------------------------------
	void Console::Draw()
	{
		engine.GetRenderer().SetRenderTarget(Renderer::ETargets::DEBUG_TARGET);
		m_inputField.Draw();

		int count = 1;
		for (int i = m_logs.size() - 1; i >= 0; i--)
		{
			int actualOffset = 0;

			if (i - m_curLineOffset < m_logs.size())
				engine.GetRenderer().DrawTextDirectly(engine.GetAssetsManager().GetFont("defaultFont"), m_logs[i - m_curLineOffset].c_str(), 25, (SCREEN_HEIGHT - 120) - 40 * count, colorWhite);

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
		if (m_logs.size() >= CONSOLE_LOGS_MAX_LENGTH)
			m_logs.erase(m_logs.begin());

		m_logs.push_back(str);
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
		if (m_history.size() >= CONSOLE_CMD_HISTORY_MAX_LENGTH)
			m_history.erase(m_history.begin());

		// Add command to history
		m_history.push_back(cmd);

		Log("> " + cmd); // log command

		// Get the input
		std::vector<std::string> input = SplitInput(cmd);

		// Process it
		if (input.size() > 0)
		{
			// Input[0] is the function name, transform it to lowercase
			std::transform(input[0].begin(), input[0].end(), input[0].begin(), [](unsigned char c) { return std::tolower(c); });

			auto it = m_cmds.find(input[0]);
			if (it != m_cmds.end())
			{
				// Command found, execute it
				return m_cmds.at(input[0]).Execute(input);
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