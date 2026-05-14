#pragma once

#include <cstdint>
#include <string>

// Needed for the assert
#include "../World/WorldCodes.h"

namespace NECRO
{
// Matches DB table structure necroauth.realmlist, parsed from CCharacterData
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
    static_assert(std::is_same_v<decltype(id), decltype(NECRO::World::CCharacterData::id)> == true);

    static_assert(std::is_same_v<decltype(characterNameLength), decltype(NECRO::World::CCharacterData::characterNameLength)> == true);
    // static_assert(std::is_same_v<decltype(characterName), decltype(NECRO::World::CCharacterData::characterName)> == true);

    static_assert(std::is_same_v<decltype(race), decltype(NECRO::World::CCharacterData::race)> == true);
    static_assert(std::is_same_v<decltype(gameClass), decltype(NECRO::World::CCharacterData::gameClass)> == true);
    static_assert(std::is_same_v<decltype(gender), decltype(NECRO::World::CCharacterData::gender)> == true);

    static_assert(std::is_same_v<decltype(level), decltype(NECRO::World::CCharacterData::level)> == true);
    static_assert(std::is_same_v<decltype(xp), decltype(NECRO::World::CCharacterData::xp)> == true);

    static_assert(std::is_same_v<decltype(zone), decltype(NECRO::World::CCharacterData::zone)> == true);
    static_assert(std::is_same_v<decltype(pos_x), decltype(NECRO::World::CCharacterData::pos_x)> == true);
    static_assert(std::is_same_v<decltype(pos_y), decltype(NECRO::World::CCharacterData::pos_y)> == true);
    static_assert(std::is_same_v<decltype(pos_z), decltype(NECRO::World::CCharacterData::pos_z)> == true);
};
}