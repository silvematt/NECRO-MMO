#include "NECROHammer.h"

#include "boost/asio.hpp"

namespace NECRO
{
namespace Hammer
{
	int Client::Init()
	{
		m_isRunning = false;

		// Load config file
		if (!Config::Instance().Load(CLIENT_CONFIG_FILE_PATH))
		{
			LOG_ERROR("Failed to load config file at: {}", CLIENT_CONFIG_FILE_PATH);

			return -1;
		}
		LOG_OK("Config file {} loaded successfully", CLIENT_CONFIG_FILE_PATH);

		ApplySettings();

		// Initialize subsystems
		m_sockManager.Initialize();

		return 0;
	}

	void Client::ApplySettings()
	{
		auto& conf = Config::Instance();

		// Apply config
		ConsoleLogger::Instance().m_logEnabled = conf.GetBool("ConsoleLoggingEnabled", true);
		FileLogger::Instance().m_logEnabled = conf.GetBool("FileLoggingEnabled", true);
	}

	void Client::Start()
	{
		m_isRunning = true;

		m_sockManager.Start();
	}

	void Client::Update()
	{
		// Updating means injecting new sockets in each network thread
		m_sockManager.Update();

		m_ioContext.run();
	}

	void Client::Stop()
	{

	}

	int Client::Shutdown()
	{
		return 0;
	}
}
}