#pragma once

#include <vector>

#include "Packet.h"
#include "AES.h"

namespace NECRO
{
    // Prevents EnlargeBufferIfNeeded to grow out of bounds
    inline constexpr size_t NETWORK_MESSAGE_ENLARGE_MAX_SIZE = Packet::DEFAULT_PCKT_SIZE * 16;

    // Upper bound on the packet size (IV + TAG + CIPHERTEXT) accepted by AESDecrypt (used in the WorldServer). 
    // Guards against oversized/hostile packets.
    inline constexpr uint32_t MAX_PACKET_SIZE_AES_DECRYPT = 512;

    //-----------------------------------------------------------------------------------------------------------
    // Higher-level view on packets, used for Network transmission.
    //-----------------------------------------------------------------------------------------------------------
    class NetworkMessage
    {
    private:

        size_t m_rpos;          // Read Pos
        size_t m_wpos;          // Write Pos 
        // ReadPos in the NetworkMessage can be viewed as "consumed" pos, 
        // if it's > 0 it means we've consumed the data until there, so it's probably a good idea to move the remaining data at beginning of the buffer with CompactData()

        std::vector<uint8_t>    m_data;          // Raw Data
        std::vector<uint8_t>    m_cipherData;    // Data after being encrpyted/decrypted
        unsigned char           m_tag[GCM_TAG_SIZE];

    public:
        // Move constructor
        NetworkMessage(NetworkMessage&& other) noexcept :
            m_rpos(other.m_rpos),
            m_wpos(other.m_wpos),
            m_data(std::move(other.m_data)),                // Move the vector
            m_cipherData(std::move(other.m_cipherData))     // Move the vector
        {
            // Copy the tag array
            std::memcpy(m_tag, other.m_tag, GCM_TAG_SIZE);
        }

        // NetworkMessage Constructor
        // data is resized (not reserved) because we'll need it as soon as this is created, and probably we'll need exactly the reservedSize amount
        NetworkMessage() : m_rpos(0), m_wpos(0)
        {
            m_data.resize(Packet::DEFAULT_PCKT_SIZE);
        }

        NetworkMessage(size_t reservedSize) : m_rpos(0), m_wpos(0)
        {
            m_data.resize(reservedSize);
        }

        // Wraps a packet in a NetworkMessage by copying its content (without invalidating the packet)
        NetworkMessage(const Packet& p) : m_rpos(0), m_wpos(0)
        {
            m_data.resize(p.Size());
            Write(p.GetContentToRead(), p.Size());
        }

        // Wraps a packet in a NetworkMessage by moving the packet's internals to the NetworkMessage's data
        NetworkMessage(Packet&& p) : m_rpos(0), m_wpos(0)
        {
            m_data = p.TakeData();
            m_wpos = m_data.size();
        }

        //-----------------------------------------------------------------------------------------------------------
        // Clears data array and write/read pos
        //-----------------------------------------------------------------------------------------------------------
        void Clear()
        {
            m_data.clear();
            m_rpos = m_wpos = 0;
        }

        //-----------------------------------------------------------------------------------------------------------
        // Resets read/write pos without clearing the array, allowing to reuse memory
        //-----------------------------------------------------------------------------------------------------------
        void SoftClear()
        {
            m_rpos = m_wpos = 0;
        }

        size_t  Size()  const { return m_data.size(); }
        bool    Empty() const { return m_data.empty(); }

        // Functions to easily access data locations
        uint8_t* GetBasePointer() { return m_data.data(); }
        uint8_t* GetReadPointer() { return GetBasePointer() + m_rpos; }
        uint8_t* GetWritePointer() { return GetBasePointer() + m_wpos; }

        // Useful information
        size_t GetActiveSize() const { return m_wpos - m_rpos; }
        size_t GetRemainingSpace() const { return m_data.size() - m_wpos; }

        // Encrpytion
        int AESEncrypt(unsigned char* key, AES::IV& iv, unsigned char* aad, int aadLen)
        {
            // Write the iv as bytes
            std::array<uint8_t, GCM_IV_SIZE> ivBytes;
            iv.ToByteArray(ivBytes);
            iv.IncrementCounter(); // increment counter here! So we are sure each encrypt operation increases the counter

            // Transform the data in this message to the encrypted equivalend, format [PCKT_SIZE | IV | TAG | CIPHERTEXT]
            m_cipherData.resize(GetActiveSize());  // same as plaintext, since GCM shouldn't expand
            int ciphertext_len = AES::Encrypt(GetReadPointer(), GetActiveSize(), aad, aadLen, key, ivBytes.data(), GCM_IV_SIZE, m_cipherData.data(), m_tag);

            if (ciphertext_len >= 0)
            {
                SoftClear();

                uint32_t packetSize = GCM_IV_SIZE + GCM_TAG_SIZE + ciphertext_len;
                packetSize = htonl(packetSize);

                Write(&packetSize, sizeof(packetSize)); // write the whole packet size as first uint32_t
                Write(ivBytes.data(), GCM_IV_SIZE);
                Write(m_tag, GCM_TAG_SIZE);
                Write(m_cipherData.data(), ciphertext_len);

                return ciphertext_len;
            }
            else
                return -1;
        }

        int AESDecrypt(unsigned char* key, unsigned char* aad, int aadLen)
        {
            if (GetActiveSize() < sizeof(uint32_t)) // not enough data to even start decrypting
                return -1;

            uint32_t packetSize;
            std::memcpy(&packetSize, GetReadPointer(), sizeof(uint32_t));
            packetSize = ntohl(packetSize); // Convert from network to host byte order

            // Check if packetSize is whitin our protocol reasonable bounds
            if (packetSize > MAX_PACKET_SIZE_AES_DECRYPT)
                return -2; // malformed packet, drop the connection

            // Check if all the packet arrived or if we're in a short send
            if (GetActiveSize() < sizeof(uint32_t) + packetSize)
                return -1; // not enough data to event start decrypting, -1 

            // Reject packets too small
            if (packetSize < GCM_IV_SIZE + GCM_TAG_SIZE)
                return -3; // malformed packet

            int cipherTextLen = static_cast<int>(packetSize - (GCM_IV_SIZE + GCM_TAG_SIZE));

            // Make sure there is content in the packet
            if (cipherTextLen < 0)
                return -4; // malformed packet

            // Read packet [PCKT_SIZE | IV | TAG | CIPHERTEXT]
            unsigned char* ivPtr = GetReadPointer() + sizeof(packetSize);
            unsigned char* tagPtr = GetReadPointer() + sizeof(packetSize) + GCM_IV_SIZE;
            unsigned char* cipherPtr = GetReadPointer() + sizeof(packetSize) + GCM_IV_SIZE + GCM_TAG_SIZE;

            // Decrypt
            m_cipherData.clear();
            m_cipherData.resize(cipherTextLen);
            int plainTextLen = AES::Decrypt(cipherPtr, cipherTextLen, aad, aadLen, tagPtr, key, ivPtr, GCM_IV_SIZE, m_cipherData.data());

            if (plainTextLen < 0)
                return -5; // decryption failed

            // Advance ReadPos
            ReadCompleted(sizeof(uint32_t) + packetSize);

            // Return plainTextLen, user will call GetDecryptedPacketPtr() to get the decrypted packet
            return plainTextLen;
        }

        //-----------------------------------------------------------------------------------------------------------------
        // When data will be processed by the socket read handler, it will have to call this function to update the rpos
        //-----------------------------------------------------------------------------------------------------------------
        void ReadCompleted(size_t bytes)
        {
            m_rpos += bytes;
        }

        //-----------------------------------------------------------------------------------------------------------------
        // When data will be written on the buffer by the recv, it will have to call this function to update the wpos
        //-----------------------------------------------------------------------------------------------------------------
        void WriteCompleted(size_t bytes)
        {
            m_wpos += bytes;
        }


        //-----------------------------------------------------------------------------------------------------------------
        // If data was consumed, shifts the remaining (unconsumed) data to the beginning of the data buffer
        //-----------------------------------------------------------------------------------------------------------------
        void CompactData()
        {
            if (m_rpos > 0)
            {
                if (m_rpos != m_wpos) // if there's data to shift
                    memmove(GetBasePointer(), GetReadPointer(), GetActiveSize());

                m_wpos = m_wpos - m_rpos; // adjust wpos accordingly
                m_rpos = 0;
            }
        }

        //-----------------------------------------------------------------------------------------------------------------
        // Writes data on the current WritePointer
        //-----------------------------------------------------------------------------------------------------------------
        void Write(void const* bytes, std::size_t size)
        {
            if (size > 0)
            {
                // Check for space
                if (GetRemainingSpace() < size)
                {
                    // Check if compacting data is enough
                    CompactData();

                    // If not, we need to resize
                    if (GetRemainingSpace() < size)
                    {
                        size_t newSize = (Size() + size);
                        newSize += (newSize / 2); // give an extra 50% of storage
                        m_data.resize(newSize);
                    }
                }

                memcpy(GetWritePointer(), bytes, size);
                m_wpos += size;
            }
        }

        //--------------------------------------------------------------------------------------------------------------------------------------------------------
        // Increases the data buffer size by 50% of the current size if it's currently full, useful to the inBuffer of the Sockets,
        // if done before a read it can avoid calling recv without any space left in the inbuffer, and therefore preventing the OS socket buffer from being drained
        //--------------------------------------------------------------------------------------------------------------------------------------------------------
        int EnlargeBufferIfNeeded()
        {
            // If size is 0 (after a Clear(), for example), this won't actually enlarge anything
            // It shouldn't happen and if it does classify it as an oversight by the developer
            if (Size() == 0)
                return -1;

            if (GetRemainingSpace() == 0)
            {
                // It's a good idea to have an upperbound for how much this can grow
                size_t newSize = Size() + (Size() / 2);
                if (newSize > NETWORK_MESSAGE_ENLARGE_MAX_SIZE)
                    return -1;

                m_data.resize(newSize);
            }

            return 0;
        }

        uint8_t* GetDecryptedPacketPtr()
        {
            return m_cipherData.data();
        }
    };
}
