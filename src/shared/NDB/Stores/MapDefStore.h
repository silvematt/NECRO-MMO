#pragma once

#include <memory>
#include "MapDef.h"

namespace NECRO
{
	// --------------------------------------------------------------------------------------
	// Class to load and store all the MapDefs only once. Maps will point to the m_defs here
	// --------------------------------------------------------------------------------------
	class MapDefStore
	{
	private:
		std::unordered_map<uint32_t, std::unique_ptr<MapDef>> m_defs;

	public:
		void			LoadAll(const NDB* mapDb);
		const MapDef*	GetDef(uint32_t mapID) const;
	};
}
