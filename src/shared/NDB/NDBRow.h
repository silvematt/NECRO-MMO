#pragma once

#include <vector>
#include <string>

#include "NDBValue.h"

namespace NECRO
{
	// ---------------------------------------------------------------------------------------------
	// A row inside of a NDB file.
	// ---------------------------------------------------------------------------------------------
	class NDBRow
	{
		friend class NDB;
		friend class NDBValue;

	private:
		std::vector<NDBValue> m_values;

	public:
		std::size_t Size() const 
		{ 
			return m_values.size(); 
		}

		// Access NDBRow[index]
		const NDBValue& operator[] (size_t col) const 
		{ 
			return m_values.at(col);
		}
	};
}
