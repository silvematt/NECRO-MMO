#ifndef NECROCONSOLE_H
#define NECROCONSOLE_H

#include <string>
#include <vector>
#include <map>

#include <fstream>

#include "Cmd.h"
#include "InputField.h"

namespace NECRO
{
namespace Client
{
	inline constexpr int	CONSOLE_MAX_LINES_PER_PAGE = 5;
	inline constexpr float	CONSOLE_FADE_OUT_TIME = 5.0f; // after 5 seconds, the messages will fade out until the console is toggled again

	// Limits
	inline constexpr int CONSOLE_LOGS_MAX_LENGTH = 128;		// To limit logs vector size
	inline constexpr int CONSOLE_CMD_HISTORY_MAX_LENGTH = 32;  // To limit command history vector size

	inline constexpr const char* CONSOLE_CMDS_LOG_FILENAME = "lastconsolecmdshistory.log";

	//----------------------------------------------------------------------------------------------
	// Console, for console commands and in-game chat
	//----------------------------------------------------------------------------------------------
	class Console
	{
	private:
		bool m_active;
		std::vector<std::string> m_logs;		// console log
		std::vector<std::string> m_history;	// contains all the commands parsed in the console

		int m_curLineOffset = 0; // offset in showing history
		int m_curHistoryIndex = -1; // to press arrow-up/down and browse history
		
		InputField m_inputField;

		std::map<std::string, Cmd>	m_cmds;			// registered commands
		std::fstream				m_cmdsLogFile;	// used to write/read commands from file, so we can save and retrieve the history of last application launch. file is CMDS_LOG_FILENAME

	private:
		int			SendCmd(const std::string& cmd);

	public:
		int			Init();
		void		Update();
		int			Shutdown();

		void		Draw();
		void		Log(const std::string& str);
		void		Toggle();

		std::vector<std::string> SplitInput(const std::string& input);

		bool		IsOpen() const;
	};

	inline bool Console::IsOpen() const
	{
		return m_active;
	}

}
}

#endif
