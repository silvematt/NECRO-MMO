#pragma once

#include <variant>
#include <string>

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

		// ---------------------------------------------------------------------------------------------------------------------------
		// Get values
		// ---------------------------------------------------------------------------------------------------------------------------
		const int* AsInt() const
		{
			return m_type == NDBValueType::INT ? &std::get<int>(m_data) : nullptr;
		}

		const float* AsFloat() const
		{
			return m_type == NDBValueType::FLOAT ? &std::get<float>(m_data) : nullptr;
		}

		const bool* AsBool() const
		{
			return m_type == NDBValueType::BOOL ? &std::get<bool>(m_data) : nullptr;
		}

		const std::string* AsString() const  
		{
			return m_type == NDBValueType::STRING ? &std::get<std::string>(m_data) : nullptr;
		}
	};
}
