#pragma once

#include <cstdint>
#include <string>

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
};
}
