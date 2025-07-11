#ifndef NECROUTILITY_H
#define NECROUTILITY_H

#include "SDL.h"

#include "string"
#include <vector>
#include <ctime>

namespace NECRO
{
namespace Client
{
	#define NECRO_MAX(a,b)            (((a) > (b)) ? (a) : (b))

namespace ClientUtility
{
	template <typename T>
	inline void RemoveFromVector(std::vector<T>& vec, size_t pos)
	{
		vec.erase(vec.begin() + pos);
	}

	inline float Clampf(float d, float min, float max)
	{
		const float t = d < min ? min : d;
		return t > max ? max : t;
	}

	inline float RandomFloat(float min, float max)
	{
		return min + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (max - min)));
	}

	inline int TryParseInt(const std::string& str)
	{
		int i = 0;

		try
		{
			i = std::stoi(str);
		}
		catch (...)
		{
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Tried to parse '%s' as INT, but the conversion failed.\n", str.c_str());
			return 0;
		}

		return i;
	}

	inline float TryParseFloat(const std::string& str)
	{
		float f = 0.0f;

		try
		{
			f = std::stof(str);
		}
		catch (...)
		{
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Tried to parse '%s' as FLOAT, but the conversion failed.\n", str.c_str());
			return 0;
		}

		return f;
	}

	inline void RemoveSpacesAndTabsFromString(std::string& str)
	{
		str.erase(std::remove(str.begin(), str.end(), ' '), str.end());
		str.erase(std::remove(str.begin(), str.end(), '\t'), str.end());
	}

	inline bool IsWhitespaceString(const std::string& str)
	{
		for (char c : str)
		{
			if (!std::isspace(c))
				return false;
		}

		return true;
	}

	inline bool IsCommentLine(const std::string& str)
	{
		bool firstCharacterFound = false;
		for (char c : str)
		{
			if (!std::isspace(c))
			{
				if (c == '#')
					return true;
				else
					return false;
			}
		}

		return false;
	}

	inline std::tm localtime_xp(std::time_t timer)
	{
		std::tm bt{};
		#if defined(__unix__)
		localtime_r(&timer, &bt);
		#elif defined(_MSC_VER)
		localtime_s(&bt, &timer);
		#else
		static std::mutex mtx;
		std::lock_guard<std::mutex> lock(mtx);
		bt = *std::localtime(&timer);
		#endif
		return bt;
	}

	// "YYYY-MM-DD HH:MM:SS"
	inline std::string time_stamp(const std::string& fmt = "%F %T")
	{
		auto bt = localtime_xp(std::time(0));
		char buf[64];
		return { buf, std::strftime(buf, sizeof(buf), fmt.c_str(), &bt) };
	}

}
}
}

#endif
