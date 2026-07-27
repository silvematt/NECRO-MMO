#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

#include "NetworkMessage.h"

namespace
{
    using namespace NECRO;

    constexpr int FRAME_HEADER_SIZE = static_cast<int>(sizeof(uint32_t)); // 4
    constexpr int IV_OFFSET         = FRAME_HEADER_SIZE;                  // 4
    constexpr int TAG_OFFSET        = FRAME_HEADER_SIZE + GCM_IV_SIZE;    // 16
    constexpr int CIPHERTEXT_OFFSET = TAG_OFFSET + GCM_TAG_SIZE;          // 32

    // Make a deterministic sessionKey
    std::array<uint8_t, AES_128_KEY_SIZE> MakeKey()
    {
        std::array<uint8_t, AES_128_KEY_SIZE> k{};
        for (int i = 0; i < AES_128_KEY_SIZE; ++i)
            k[i] = static_cast<uint8_t>(i + 1);
        return k;
    }

    // Read packetSize field (network byte order) at the start of a frame
    uint32_t ReadFrameSize(const uint8_t* frame)
    {
        uint32_t s;
        std::memcpy(&s, frame, sizeof(s));
        return ntohl(s);
    }

    // Pull the 8-byte counter portion of an IV from a frame
    uint64_t ReadFrameIVCounter(const uint8_t* frame)
    {
        uint64_t c;
        std::memcpy(&c, frame + IV_OFFSET + sizeof(uint32_t), sizeof(c));
        return c;
    }

    // Build an encrypted frame in "out", returns frame size
    size_t EncryptFrame(NetworkMessage& out,
                        const std::vector<uint8_t>& plaintext,
                        std::array<uint8_t, AES_128_KEY_SIZE>& key,
                        AES::IV& iv,
                        std::vector<uint8_t>& aad)
    {
        out.Write(plaintext.data(), plaintext.size());
        int ct_len = out.AESEncrypt(key.data(), iv, aad.data(), static_cast<int>(aad.size()));
        EXPECT_GE(ct_len, 0);
        return out.GetActiveSize();
    }
}

TEST(NetworkMessage, RoundtripRecoversPlaintext)
{
    auto key = MakeKey();
    AES::IV iv;
    std::vector<uint8_t> aad = { 'O', 'P', 0x01, 0x00 };

    std::vector<uint8_t> plaintext = 
    {
        'h','e','l','l','o',' ','w','o','r','l','d','!',1,2,3,4
    };

    NetworkMessage sender;
    size_t frame_size = EncryptFrame(sender, plaintext, key, iv, aad);

    // Move the framed bytes into a receiver as if recv() delivered them.
    NetworkMessage receiver;
    receiver.Write(sender.GetReadPointer(), frame_size);

    int pt_len = receiver.AESDecrypt(key.data(), aad.data(), static_cast<int>(aad.size()));
    ASSERT_EQ(pt_len, static_cast<int>(plaintext.size()));
    EXPECT_EQ(0, std::memcmp(receiver.GetDecryptedPacketPtr(), plaintext.data(), plaintext.size()));

    // After decrypt, the entire frame should be consumed (rpos == wpos)
    EXPECT_EQ(receiver.GetActiveSize(), 0u);
}

TEST(NetworkMessage, FrameLayoutIsSizeIvTagCiphertext)
{
    auto key = MakeKey();
    AES::IV iv;
    std::vector<uint8_t> aad;
    std::vector<uint8_t> plaintext(40, 0x55);

    NetworkMessage sender;
    size_t frame_size = EncryptFrame(sender, plaintext, key, iv, aad);

    // Total frame: 4 (size) + 12 (IV) + 16 (tag) + ciphertext
    ASSERT_EQ(frame_size, FRAME_HEADER_SIZE + GCM_IV_SIZE + GCM_TAG_SIZE + plaintext.size());

    // The size field encodes IV+TAG+CT (NOT including the 4-byte length prefix itself)
    uint32_t declared = ReadFrameSize(sender.GetReadPointer());
    EXPECT_EQ(declared, GCM_IV_SIZE + GCM_TAG_SIZE + plaintext.size());

    // Ciphertext region must differ from the plaintext (sanity that encryption ran)
    EXPECT_NE(0, std::memcmp(sender.GetReadPointer() + CIPHERTEXT_OFFSET, plaintext.data(), plaintext.size()));
}

TEST(NetworkMessage, IVCounterIncrementsAcrossEncrypts)
{
    auto key = MakeKey();
    AES::IV iv;
    std::vector<uint8_t> aad;
    std::vector<uint8_t> plaintext(8, 0x01);

    NetworkMessage a;
    EncryptFrame(a, plaintext, key, iv, aad);
    uint64_t counter_a = ReadFrameIVCounter(a.GetReadPointer());

    NetworkMessage b;
    EncryptFrame(b, plaintext, key, iv, aad);
    uint64_t counter_b = ReadFrameIVCounter(b.GetReadPointer());

    EXPECT_EQ(counter_b, counter_a + 1);

    // The 4-byte random prefix must stay stable across encrypts on the same IV.
    EXPECT_EQ(0, std::memcmp(a.GetReadPointer() + IV_OFFSET, b.GetReadPointer() + IV_OFFSET, sizeof(uint32_t)));
}

TEST(NetworkMessage, TamperedCiphertextReturnsAuthFailure)
{
    auto key = MakeKey();
    AES::IV iv;
    std::vector<uint8_t> aad;
    std::vector<uint8_t> plaintext = { 's','e','c','r','e','t' };

    NetworkMessage sender;
    size_t frame_size = EncryptFrame(sender, plaintext, key, iv, aad);

    NetworkMessage receiver;
    receiver.Write(sender.GetReadPointer(), frame_size);

    receiver.GetBasePointer()[CIPHERTEXT_OFFSET] ^= 0x01;

    EXPECT_EQ(receiver.AESDecrypt(key.data(), aad.data(), static_cast<int>(aad.size())), -5);
}

TEST(NetworkMessage, WrongAADReturnsAuthFailure)
{
    auto key = MakeKey();
    AES::IV iv;
    std::vector<uint8_t> aad      = { 'A','B','C' };
    std::vector<uint8_t> wrongAad = { 'A','B','D' };
    std::vector<uint8_t> plaintext = { 'w','o','w' };

    NetworkMessage sender;
    size_t frame_size = EncryptFrame(sender, plaintext, key, iv, aad);

    NetworkMessage receiver;
    receiver.Write(sender.GetReadPointer(), frame_size);

    EXPECT_EQ(receiver.AESDecrypt(key.data(), wrongAad.data(), static_cast<int>(wrongAad.size())), -5);
}

TEST(NetworkMessage, ShortReadReturnsMinusOneAndPreservesData)
{
    auto key = MakeKey();
    AES::IV iv;
    std::vector<uint8_t> aad;
    std::vector<uint8_t> plaintext(40, 0x77);

    NetworkMessage sender;
    size_t frame_size = EncryptFrame(sender, plaintext, key, iv, aad);

    NetworkMessage receiver;
    // Deliver the frame minus its last byte to simulate a TCP short read
    receiver.Write(sender.GetReadPointer(), frame_size - 1);

    EXPECT_EQ(receiver.AESDecrypt(key.data(), aad.data(), static_cast<int>(aad.size())), -1);

    // rpos must NOT have advanced the buffered partial frame must remain intact so the next recv() can append the missing bytes
    EXPECT_EQ(receiver.GetActiveSize(), frame_size - 1);
}

TEST(NetworkMessage, OversizedFrameRejected)
{
    auto key = MakeKey();
    std::vector<uint8_t> aad;

    NetworkMessage receiver;
    uint32_t bogusSize = htonl(NECRO::MAX_PACKET_SIZE_AES_DECRYPT+1); // > MAX_PACKET_SIZE_AES_DECRYPT (512)
    receiver.Write(&bogusSize, sizeof(bogusSize));

    EXPECT_EQ(receiver.AESDecrypt(key.data(), aad.data(), static_cast<int>(aad.size())), -2);
}

TEST(NetworkMessage, UndersizedFrameRejected)
{
    auto key = MakeKey();
    std::vector<uint8_t> aad;

    NetworkMessage receiver;

    // Declared size smaller than IV+TAG (28) can't possibly hold a valid GCM frame
    uint32_t bogusSize = htonl(GCM_IV_SIZE + GCM_TAG_SIZE - 1);
    receiver.Write(&bogusSize, sizeof(bogusSize));
    std::vector<uint8_t> filler(GCM_IV_SIZE + GCM_TAG_SIZE - 1, 0x00);
    receiver.Write(filler.data(), filler.size());

    EXPECT_EQ(receiver.AESDecrypt(key.data(), aad.data(), static_cast<int>(aad.size())), -3);
}

TEST(NetworkMessage, TwoFramesInBufferDecryptSequentially)
{
    auto key = MakeKey();
    AES::IV iv;
    std::vector<uint8_t> aad;

    std::vector<uint8_t> p1 = { 'f','i','r','s','t' };
    std::vector<uint8_t> p2 = { 's','e','c','o','n','d' };

    NetworkMessage s1, s2;
    size_t f1 = EncryptFrame(s1, p1, key, iv, aad);
    size_t f2 = EncryptFrame(s2, p2, key, iv, aad);

    NetworkMessage receiver;
    receiver.Write(s1.GetReadPointer(), f1);
    receiver.Write(s2.GetReadPointer(), f2);

    int pt1 = receiver.AESDecrypt(key.data(), aad.data(), static_cast<int>(aad.size()));
    ASSERT_EQ(pt1, static_cast<int>(p1.size()));
    EXPECT_EQ(0, std::memcmp(receiver.GetDecryptedPacketPtr(), p1.data(), p1.size()));

    // After consuming frame 1, exactly frame 2 should remain
    EXPECT_EQ(receiver.GetActiveSize(), f2);

    int pt2 = receiver.AESDecrypt(key.data(), aad.data(), static_cast<int>(aad.size()));
    ASSERT_EQ(pt2, static_cast<int>(p2.size()));
    EXPECT_EQ(0, std::memcmp(receiver.GetDecryptedPacketPtr(), p2.data(), p2.size()));

    EXPECT_EQ(receiver.GetActiveSize(), 0u);
}
