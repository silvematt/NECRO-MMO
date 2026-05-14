#pragma once

#include <cstdint>
#include <string>

namespace NECRO
{
// Matches DB table structure necroauth.realmlist, parsed from CCharacterData
struct CharacterData
{
    uint16_t id;

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
};
}