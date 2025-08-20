#include "Config.h"

#include "ConsoleLogger.h"
#include "FileLogger.h"

#include <fstream>
#include <iostream>
#include <algorithm>

namespace NECRO
{
	bool Config::Load(const std::string& filePath)
	{
		m_confMap.clear();

		// Read the provided file
		std::ifstream file(filePath);

		if (!file)
			return false;

		// Read the file
		std::string line;
		while (std::getline(file, line))
		{
			// Skip comments
			if (line.empty() || line[0] == '#')
				continue;

			// Delete spaces
			line.erase(remove_if(line.begin(), line.end(), [](unsigned char c) { return std::isspace(c); }), line.end());

			// Get field's name
			std::string vName = line.substr(0, line.find('='));
			std::string vValue = line.substr(line.find('=')+1);

			m_confMap.emplace(vName, vValue);
		}

		// Print it
		LOG_INFO("CONFIG: Loaded {} - Printing Table:", filePath);
		for (auto& v : m_confMap)
		{
			LOG_INFO("{} = {}", v.first, v.second);
		}
		LOG_INFO("CONFIG: END");

		return true;
	}

	int Config::GetInt(const std::string& q, int fallback)
	{
		auto it = m_confMap.find(q);
		if (it == m_confMap.end())
		{
			// Return default
			LOG_WARNING("Tried to load {} from Config, but it was never loaded. Returning fallback: {}", q, fallback);
			return fallback;
		}

		return std::stoi(m_confMap[q]);
	}

	float Config::GetFloat(const std::string& q, float fallback)
	{
		auto it = m_confMap.find(q);
		if (it == m_confMap.end())
		{
			// Return default
			LOG_WARNING("Tried to load {} from Config, but it was never loaded. Returning fallback: {}", q, fallback);
			return fallback;
		}

		return std::stof(m_confMap[q]);
	}

	bool Config::GetBool(const std::string& q, bool fallback)
	{
		auto it = m_confMap.find(q);
		if (it == m_confMap.end())
		{
			// Return default
			LOG_WARNING("Tried to load {} from Config, but it was never loaded. Returning fallback: {}", q, fallback);
			return fallback;
		}

		return m_confMap[q] != "0";
	}

	std::string Config::GetString(const std::string& q, std::string fallback)
	{
		auto it = m_confMap.find(q);
		if (it == m_confMap.end())
		{
			// Return default
			LOG_WARNING("Tried to load {} from Config, but it was never loaded. Returning fallback: {}", q, fallback);
			return fallback;
		}

		return m_confMap[q];
	}

}
