#pragma once

#include <cstdint>
#include <string>

// Matches DB table structure necroauth.realmlist: TODO unify Realm and RealmEntry on the client side
struct Realm
{
	uint32_t ID;
	std::string name;
	std::string ip;
	uint16_t port;
	uint8_t status;
};
