#pragma once

#include <string>
#include <ctime>
#include <mutex>

namespace NECRO
{
namespace Utility
{
    inline std::tm localtime_xp(std::time_t timer)
    {
        std::tm bt{};
        #if defined(__unix__)
            localtime_r(&timer, &bt);
        #elif defined(_MSC_VER)
        localtime_s(&bt, &timer);
        #else
            static std::mutex mtx;
            std::lock_guard<std::mutex> lock(mtx);
            bt = *std::localtime(&timer);
        #endif
        return bt;
    }

    // "YYYY-MM-DD HH:MM:SS"
    inline std::string time_stamp(const std::string& fmt = "%F %T")
    {
        auto bt = localtime_xp(std::time(0));
        char buf[64];
        return { buf, std::strftime(buf, sizeof(buf), fmt.c_str(), &bt) };
    }

    // Returns true if hash has at least bits-leading zero bits.
    inline bool ProofOfWork_HasLeadingZeroBits(const uint8_t* hash, uint32_t bits)
    {
        uint32_t fullBytes = bits / 8;
        uint32_t extraBits = bits % 8;

        for (uint32_t i = 0; i < fullBytes; i++)
            if (hash[i] != 0)
                return false;

        if (extraBits == 0)
            return true;

        // The next byte must have extraBits zero bits at the top
        uint8_t mask = 0xFF << (8 - extraBits);
        return (hash[fullBytes] & mask) == 0;
    }
}
}
