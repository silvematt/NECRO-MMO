#include "NECROHammer.h"

namespace NECRO
{
namespace Hammer
{
	Client g_client;

	int Client::Init()
	{
		m_isRunning = false;

		return 0;
	}

	void Client::Start()
	{
		m_isRunning = true;
	}

	void Client::Update()
	{

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