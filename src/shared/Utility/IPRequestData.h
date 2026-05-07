#pragma once
#include <chrono>


// IP-based spam prevention <ip, last attempt>
struct IPRequestData
{
	std::chrono::steady_clock::time_point lastUpdate;
	size_t tries;
};

// IP-Request spam prevention
// Usage:
//	std::unordered_map<std::string, IPRequestData> m_ipRequestMap;