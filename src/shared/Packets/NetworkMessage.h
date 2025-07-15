#ifndef NETWORK_MESSAGE_H
#define NETWORK_MESSAGE_H

#include <vector>

#include "Packet.h"
#include "AES.h"

namespace NECRO
{
    //-----------------------------------------------------------------------------------------------------------
    // Higher-level view packets, used for Network transmission.
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

        // Wraps a packet in a NetworkMessage
        NetworkMessage(const Packet& p)
        {
            m_data.resize(p.Size());
            Write(p.GetContentToRead(), p.Size());
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
            if (GetActiveSize() < sizeof(uint32_t)) // not enough data to event start decrypting
                return -1;

            uint32_t packetSize;
            std::memcpy(&packetSize, GetReadPointer(), sizeof(uint32_t));
            packetSize = ntohl(packetSize); // Convert from network to host byte order

            if (GetActiveSize() < sizeof(uint32_t) + packetSize)
                return -1; // not enough data to event start decrypting

            // Read packet [PCKT_SIZE | IV | TAG | CIPHERTEXT]
            unsigned char* ivPtr = GetReadPointer() + sizeof(packetSize);
            unsigned char* tagPtr = GetReadPointer() + sizeof(packetSize) + GCM_IV_SIZE;
            unsigned char* cipherPtr = GetReadPointer() + sizeof(packetSize) + GCM_IV_SIZE + GCM_TAG_SIZE;
            int cipherTextLen = packetSize - (GCM_IV_SIZE + GCM_TAG_SIZE);

            if (cipherTextLen < 0)
                return -2; // malformed packet

            // Decrypt
            m_cipherData.resize(cipherTextLen);
            int plainTextLen = AES::Decrypt(cipherPtr, cipherTextLen, aad, aadLen, tagPtr, key, ivPtr, GCM_IV_SIZE, m_cipherData.data());

            if (plainTextLen < 0)
                return -3; // decryption failed

            // Replace internal buffer with plaintext
            SoftClear();
            Write(m_cipherData.data(), plainTextLen);

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

        //---------------------------------------------------------------------------------------------------------------------------
        // Increases the data buffer size by 50% of the current size if it's currently full, useful to the inBuffer of the Sockets,
        // if done before a read it can avoid having to queue packets
        //---------------------------------------------------------------------------------------------------------------------------
        void EnlargeBufferIfNeeded()
        {
            if (GetRemainingSpace() == 0)
                m_data.resize(Size() + (Size() / 2));
        }

    };
}
#endif
