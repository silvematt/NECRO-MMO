#ifndef NECROCMD_H
#define NECROCMD_H

#include <vector>
#include <string>

//----------------------------------------------------------------------------------------------
// Wraps a command that can be run in the console.
//----------------------------------------------------------------------------------------------
class Cmd
{
public:
	int (Cmd::* routine)(const std::vector<std::string>&);

public:
	Cmd(int (Cmd::* r)(const std::vector<std::string>&)) :
		routine(r)
	{
	}

	int		Execute(const std::vector<std::string>& args);

	int		Cmd_Help(const std::vector<std::string>& args);
	int		Cmd_TeleportToGrid(const std::vector<std::string>& args);
	int		Cmd_NoClip(const std::vector<std::string>& args);
	int		Cmd_QuitApplication(const std::vector<std::string>& args);

	int		Cmd_ConnectToAuthServer(const std::vector<std::string>& args);

	// Debug
	int		Cmd_ToggleCollisionDebug(const std::vector<std::string>& args);
	int		Cmd_ToggleOcclusionDebug(const std::vector<std::string>& args);

};

#endif
