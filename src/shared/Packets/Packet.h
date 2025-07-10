#ifndef PACKET_H
#define PACKET_H

#include <vector>
#include <string>
#include <stdexcept>

//----------------------------------------------------------------------------------------------------------------
// Low-level definition of packet, suited for creation and manipulation of packets to then send as NetworkMessages
//----------------------------------------------------------------------------------------------------------------
class Packet
{
private:
    size_t rpos, wpos;          // Read Pos, Write Pos
    std::vector<uint8_t> data;  // Raw Data

public:
    constexpr static size_t DEFAULT_PCKT_SIZE = 0x1000;

    // Packet Constructor
    Packet() : rpos(0), wpos(0)
    {
        // Data is reserved, not allocated. Reserving data allows to avoid stress on the allocator when appending bytes to the packet.
        // This means when we'll send the packet to the network, we'll still use 'data.size()' which will only contain the bytes we've actually written and need to send.
        data.reserve(DEFAULT_PCKT_SIZE);
    }

    Packet(size_t reservedSize) : rpos(0), wpos(0)
    {
        data.reserve(reservedSize);
    }

    void Clear()
    {
        data.clear();
        wpos = wpos = 0;
    }

    uint8_t* GetContent()
    { 
        if (data.empty())
            throw std::out_of_range("Trying to GetContent of an empty packet!");

        return data.data(); 
    }

    size_t  Size()  const   { return data.size();   }
    bool    Empty() const   { return data.empty();  }


    // Base append function
    void Append(uint8_t const * src, size_t cnt);

    // Template allows us to override operators and append different data types
    template <typename T> void Append(T value)
    {
        Append((uint8_t*)&value, sizeof(value));
    }

    void Print() const;

#pragma region OPERATORS_OVERLOAD

    Packet& operator<<(bool value)
    {
        Append<uint8_t>(value ? 1 : 0);
        return *this; // allows for method chaining
    }

    Packet& operator<<(uint8_t value)
    {
        Append<uint8_t>(value);
        return *this;
    }

    Packet& operator<<(uint16_t value)
    {
        Append<uint16_t>(value);
        return *this;
    }

    Packet& operator<<(uint32_t value)
    {
        Append<uint32_t>(value);
        return *this;
    }

    Packet& operator<<(uint64_t value)
    {
        Append<uint64_t>(value);
        return *this;
    }

    Packet& operator<<(int8_t value)
    {
        Append<int8_t>(value);
        return *this;
    }

    Packet& operator<<(int16_t value)
    {
        Append<int16_t>(value);
        return *this;
    }

    Packet& operator<<(int32_t value)
    {
        Append<int32_t>(value);
        return *this;
    }

    Packet& operator<<(int64_t value)
    {
        Append<int64_t>(value);
        return *this;
    }

    Packet& operator<<(float value)
    {
        Append<float>(value);
        return *this;
    }

    Packet& operator<<(double value)
    {
        Append<double>(value);
        return *this;
    }

    Packet& operator<<(std::string const& value)
    {
        size_t length = value.length();
        Append(reinterpret_cast<const uint8_t*>(value.data()), length);
        //Append(static_cast<uint8_t>(0)); // '\0' DO NOT APPEND NULL TERMINATOR!
        return *this;
    }

    Packet& operator<<(const char* value)
    {
        size_t length = std::strlen(value);
        Append(reinterpret_cast<const uint8_t*>(value), length);
        Append(static_cast<uint8_t>(0)); // '\0'
        return *this;
    }


#pragma endregion

};

#endif
