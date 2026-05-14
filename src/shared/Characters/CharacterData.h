#pragma once

#include <cstdint>
#include <string>

// Needed for the assert
#include "../World/WorldCodes.h"

namespace NECRO
{
// Matches DB table structure necroauth.realmlist, parsed from CharacterDataOnWire
struct CharacterData
{
    uint32_t id;

    uint8_t characterNameLength;
    std::string characterName;

    uint8_t race;
    uint8_t gameClass;
    uint8_t gender;

    uint8_t level;
    uint32_t xp;

    uint8_t zone;
    float_t pos_x;
    float_t pos_y;
    float_t pos_z;

    // Assert types are the same as Wire packets
    static_assert(std::is_same_v<decltype(id), decltype(NECRO::World::CharacterDataOnWire::id)> == true);

    static_assert(std::is_same_v<decltype(characterNameLength), decltype(NECRO::World::CharacterDataOnWire::characterNameLength)> == true);
    // static_assert(std::is_same_v<decltype(characterName), decltype(NECRO::World::CharacterDataOnWire::characterName)> == true);

    static_assert(std::is_same_v<decltype(race), decltype(NECRO::World::CharacterDataOnWire::race)> == true);
    static_assert(std::is_same_v<decltype(gameClass), decltype(NECRO::World::CharacterDataOnWire::gameClass)> == true);
    static_assert(std::is_same_v<decltype(gender), decltype(NECRO::World::CharacterDataOnWire::gender)> == true);

    static_assert(std::is_same_v<decltype(level), decltype(NECRO::World::CharacterDataOnWire::level)> == true);
    static_assert(std::is_same_v<decltype(xp), decltype(NECRO::World::CharacterDataOnWire::xp)> == true);

    static_assert(std::is_same_v<decltype(zone), decltype(NECRO::World::CharacterDataOnWire::zone)> == true);
    static_assert(std::is_same_v<decltype(pos_x), decltype(NECRO::World::CharacterDataOnWire::pos_x)> == true);
    static_assert(std::is_same_v<decltype(pos_y), decltype(NECRO::World::CharacterDataOnWire::pos_y)> == true);
    static_assert(std::is_same_v<decltype(pos_z), decltype(NECRO::World::CharacterDataOnWire::pos_z)> == true);
};
}