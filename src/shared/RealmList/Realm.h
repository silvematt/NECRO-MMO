#pragma once

#include <cstdint>
#include <string>

// Needed for the assert
#include "../Authentication/AuthCodes.h"

namespace NECRO
{
// Matches DB table structure necroauth.realmlist, parsed from RealmDataOnWire
struct Realm
{
	uint32_t ID;
	uint8_t status;
	std::string ip;
	uint16_t port;
	std::string name;

	// Assert types are the same as Wire packets
	static_assert(std::is_same_v<decltype(ID), decltype(NECRO::Auth::RealmDataOnWire::id)> == true);
	static_assert(std::is_same_v<decltype(status), decltype(NECRO::Auth::RealmDataOnWire::status)> == true);
	// static_assert(std::is_same_v<decltype(ip), decltype(NECRO::Auth::RealmDataOnWire::ipAddress)> == true);
	static_assert(std::is_same_v<decltype(port), decltype(NECRO::Auth::RealmDataOnWire::port)> == true);
	// static_assert(std::is_same_v<decltype(name), decltype(NECRO::Auth::RealmDataOnWire::name)> == true);
};
}
