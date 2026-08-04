#pragma once

#include <cstdint>

namespace NECRO
{
namespace World
{
	class Entity
	{
	private:
		uint64_t	m_guid;
		bool		m_isActive;

	public:
		Entity(uint64_t guid) : m_guid(guid), m_isActive(true) {}
	};
}
}
