#pragma once

#include <variant>
#include <string>
#include <iostream>

namespace NECRO
{
	enum class NDBValueType
	{
		INT,
		FLOAT,
		BOOL,
		STRING
	};

	// ---------------------------------------------------------------------------------------------------------------------------
	// A value inside of a NecroDatabase. It's similar to a QueryResult we would have gotten if we would have used a standard DB
	// ---------------------------------------------------------------------------------------------------------------------------
	class NDBValue
	{
	private:
		NDBValueType m_type;
		std::variant<int, float, bool, std::string> m_data;

	public:
		explicit NDBValue(int v) : m_type(NDBValueType::INT), m_data(v){}
		explicit NDBValue(float v) : m_type(NDBValueType::FLOAT), m_data(v){}
		explicit NDBValue(bool v) : m_type(NDBValueType::BOOL), m_data(v){}
		explicit NDBValue(std::string v) : m_type(NDBValueType::STRING), m_data(std::move(v)){}
		explicit NDBValue(const char* v) : m_type(NDBValueType::STRING), m_data(std::string(v)) {}

		NDBValueType GetType() const 
		{ 
			return m_type; 
		}


		bool TryGetInt(int& out) const
		{
			if (m_type != NDBValueType::INT)
				return false;

			out = std::get<int>(m_data);
			return true;
		}

		bool TryGetFloat(float& out) const
		{
			if (m_type != NDBValueType::FLOAT)
				return false;

			out = std::get<float>(m_data);
			return true;
		}

		bool TryGetBool(bool& out) const
		{
			if (m_type != NDBValueType::BOOL)
				return false;

			out = std::get<bool>(m_data);
			return true;
		}

		bool TryGetString(std::string& out) const
		{
			if (m_type != NDBValueType::STRING)
				return false;

			out = std::get<std::string>(m_data);
			return true;
		}
	};
}
