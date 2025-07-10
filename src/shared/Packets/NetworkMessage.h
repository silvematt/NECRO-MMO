#ifndef NETWORK_MESSAGE_H
#define NETWORK_MESSAGE_H

#include <vector>

#include "Packet.h"
#include "AES.h"

//-----------------------------------------------------------------------------------------------------------
// Higher-level view packets, used for Network transmission.
//-----------------------------------------------------------------------------------------------------------
class NetworkMessage
{
private:
    size_t rpos, wpos;          // Read Pos, Write Pos. 
                                // ReadPos in the NetworkMessage can be viewed as "consumed" pos, 
                                // if it's > 0 it means we've consumed the data until there, so it's probably a good idea to move the remaining data at beginning of the buffer with CompactData()

    std::vector<uint8_t> data;          // Raw Data
    std::vector<uint8_t> cipherData;    // Data after being encrpyted/decrypted
    unsigned char tag[GCM_TAG_SIZE];

public:
    // NetworkMessage Constructor
    // data is resized (not reserved) because we'll need it as soon as this is created, and probably we'll need exactly the reservedSize amount
    NetworkMessage() : rpos(0), wpos(0)
    {
        data.resize(Packet::DEFAULT_PCKT_SIZE);
    }

    NetworkMessage(size_t reservedSize) : rpos(0), wpos(0)
    {
        data.resize(reservedSize);
    }

    // Wraps a packet in a NetworkMessage
    NetworkMessage(Packet p)
    {
        data.resize(p.Size());
        Write(p.GetContent(), p.Size());
    }

    //-----------------------------------------------------------------------------------------------------------
    // Clears data array and write/read pos
    //-----------------------------------------------------------------------------------------------------------
    void Clear()
    {
        data.clear();
        rpos = wpos = 0;
    }

    //-----------------------------------------------------------------------------------------------------------
    // Resets read/write pos without clearing the array, allowing to reuse memory
    //-----------------------------------------------------------------------------------------------------------
    void SoftClear()
    {
        rpos = wpos = 0;
    }

    size_t  Size()  const { return data.size(); }
    bool    Empty() const { return data.empty(); }

    // Functions to easily access data locations
    uint8_t* GetBasePointer()   { return data.data(); }
    uint8_t* GetReadPointer()   { return GetBasePointer() + rpos; }
    uint8_t* GetWritePointer()  { return GetBasePointer() + wpos; }

    // Useful information
    size_t GetActiveSize() const { return wpos - rpos; }
    size_t GetRemainingSpace() const { return data.size() - wpos; }

    // Encrpytion
    int AESEncrypt(unsigned char* key, NECROAES::IV& iv, unsigned char* aad, int aadLen)
    {
        // Write the iv as bytes
        std::array<uint8_t, GCM_IV_SIZE> ivBytes;
        iv.ToByteArray(ivBytes);
        iv.IncrementCounter(); // increment counter here! So we are sure each encrypt operation increases the counter

        // Transform the data in this message to the encrypted equivalend, format [PCKT_SIZE | IV | TAG | CIPHERTEXT]
        cipherData.resize(GetActiveSize());  // same as plaintext, since GCM shouldn't expand
        int ciphertext_len = NECROAES::Encrypt(GetReadPointer(), GetActiveSize(), aad, aadLen, key, ivBytes.data(), GCM_IV_SIZE, cipherData.data(), tag);

        if (ciphertext_len >= 0)
        {
            SoftClear();

            uint32_t packetSize = GCM_IV_SIZE + GCM_TAG_SIZE + ciphertext_len;
            packetSize = htonl(packetSize);

            Write(&packetSize, sizeof(packetSize)); // write the whole packet size as first uint32_t
            Write(ivBytes.data(), GCM_IV_SIZE);
            Write(tag, GCM_TAG_SIZE);
            Write(cipherData.data(), ciphertext_len);

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
        cipherData.resize(cipherTextLen);
        int plainTextLen = NECROAES::Decrypt(cipherPtr, cipherTextLen, aad, aadLen, tagPtr, key, ivPtr, GCM_IV_SIZE, cipherData.data());

        if (plainTextLen < 0)
            return -3; // decryption failed

        // Replace internal buffer with plaintext
        SoftClear();
        Write(cipherData.data(), plainTextLen);

        return plainTextLen;
    }

    //-----------------------------------------------------------------------------------------------------------------
    // When data will be processed by the socket read handler, it will have to call this function to update the rpos
    //-----------------------------------------------------------------------------------------------------------------
    void ReadCompleted(size_t bytes)
    { 
        rpos += bytes; 
    }

    //-----------------------------------------------------------------------------------------------------------------
    // When data will be written on the buffer by the recv, it will have to call this function to update the wpos
    //-----------------------------------------------------------------------------------------------------------------
    void WriteCompleted(size_t bytes) 
    { 
       wpos += bytes; 
    }


    //-----------------------------------------------------------------------------------------------------------------
    // If data was consumed, shifts the remaining (unconsumed) data to the beginning of the data buffer
    //-----------------------------------------------------------------------------------------------------------------
    void CompactData()
    {
        if (rpos > 0)
        {
            if (rpos != wpos) // if there's data to shift
                memmove(GetBasePointer(), GetReadPointer(), GetActiveSize());

            wpos = wpos - rpos; // adjust wpos accordingly
            rpos = 0;
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
                    data.resize(newSize);
                }
            }

            memcpy(GetWritePointer(), bytes, size);
            wpos += size;
        }
    }

    //---------------------------------------------------------------------------------------------------------------------------
    // Increases the data buffer size by 50% of the current size if it's currently full, useful to the inBuffer of the Sockets,
    // if done before a read it can avoid having to queue packets
    //---------------------------------------------------------------------------------------------------------------------------
    void EnlargeBufferIfNeeded()
    {
        if (GetRemainingSpace() == 0)
            data.resize(Size() + (Size() / 2));
    }

};

#endif
