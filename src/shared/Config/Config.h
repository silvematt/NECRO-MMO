#ifndef NECRO_AUTH_SERV_CONFIG_H
#define NECRO_AUTH_SERV_CONFIG_H

#include <unordered_map>
#include <string>

namespace NECRO
{
	class Config
	{
	private:
		std::unordered_map<std::string, std::string> m_confMap;

	public:
		static Config& Instance()
		{
			static Config conf;
			return conf;
		}

		bool			Load(const std::string& filePath);

		int				GetInt(const std::string& key, int fallback);
		float			GetFloat(const std::string& key, float fallback);
		bool			GetBool(const std::string& key, bool fallback);
		std::string		GetString(const std::string& key, std::string fallback);
	};
}

#endif
