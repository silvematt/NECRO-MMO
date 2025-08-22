#include "NECROHammer.h"

#include "boost/asio.hpp"

namespace NECRO
{
namespace Hammer
{
	// ------------------------------------------------------------------
	// Initializes the Hammer client
	// ------------------------------------------------------------------
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

	// -----------------------------------------------------------------------
	// Applies the Config settings from the config file loaded during Init
	// -----------------------------------------------------------------------
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

		// Posts work on the main context_io and makes up the main loop
		m_sockManager.Start();

		// Start
		m_sockManager.StartThreads();
	}

	void Client::Update()
	{
		m_ioContext.run();

		// Here if somebody called Client::Stop() or the m_ioContext ran out of work
		Shutdown();
	}

	void Client::Stop()
	{
		m_ioContext.stop();
	}

	int Client::Shutdown()
	{
		// Stop and join the threads
		m_sockManager.StopThreads();
		m_sockManager.JoinThreads();

		return 0;
	}
}
}