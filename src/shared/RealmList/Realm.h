#pragma once

#include <cstdint>
#include <string>

// Needed for the assert
#include "../Authentication/AuthCodes.h"

namespace NECRO
{
// Matches DB table structure necroauth.realmlist, parsed from CRealmData
struct Realm
{
	uint32_t ID;
	uint8_t status;
	std::string ip;
	uint16_t port;
	std::string name;

	// Assert types are the same as Wire packets
	static_assert(std::is_same_v<decltype(ID), decltype(NECRO::Auth::CRealmData::id)> == true);
	static_assert(std::is_same_v<decltype(status), decltype(NECRO::Auth::CRealmData::status)> == true);
	// static_assert(std::is_same_v<decltype(ip), decltype(NECRO::Auth::CRealmData::ipAddress)> == true);
	static_assert(std::is_same_v<decltype(port), decltype(NECRO::Auth::CRealmData::port)> == true);
	// static_assert(std::is_same_v<decltype(name), decltype(NECRO::Auth::CRealmData::name)> == true);
};
}
