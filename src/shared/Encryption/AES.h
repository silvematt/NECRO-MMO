#pragma once

#include <array>
#include <stdexcept>

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>

namespace NECRO
{
	inline constexpr int AES_128_KEY_SIZE = 16;	// 128-bit key
	inline constexpr int GCM_IV_SIZE = 12;
	inline constexpr int GCM_TAG_SIZE = 16;

	namespace AES
	{
		// GCM IV
		struct IV
		{
			uint32_t prefix;	// 4 bytes random prefix
			uint64_t counter;	// 8 bytes counter

			IV()
			{
				RandomizePrefix();
				ResetCounter();
			}

			void RandomizePrefix()
			{
				if (RAND_bytes(reinterpret_cast<unsigned char*>(&prefix), sizeof(prefix)) != 1)
				{
					char buf[256];
					unsigned long err = ERR_get_error();
					ERR_error_string_n(err, buf, sizeof(buf));
					throw std::runtime_error(std::string("Failed to randomize IV Prefix via RAND_bytes: ") + buf);
				}
			}

			void ResetCounter()
			{
				counter = 0;
			}

			void IncrementCounter()
			{
				counter++;
			}

			void ToByteArray(std::array<uint8_t, GCM_IV_SIZE>& out)
			{
				// Copy prefix (4 bytes)
				std::memcpy(out.data(), &prefix, sizeof(prefix));

				// Copy counter (8 bytes) after prefix
				std::memcpy(out.data() + sizeof(prefix), &counter, sizeof(counter));
			}
		};

		inline std::array<uint8_t, AES_128_KEY_SIZE> GenerateSessionKey()
		{
			std::array<uint8_t, AES_128_KEY_SIZE> k{};
			if (RAND_bytes(k.data(), AES_128_KEY_SIZE) != 1)
			{
				throw std::runtime_error("Failed to generate AES session key");
			}

			return k;
		}

		// --------------------------------------------------------------
		// EVP_CIPHER_CTX per thread unique per thread
		// --------------------------------------------------------------
		inline EVP_CIPHER_CTX* AcquireThreadCtx()
		{
			struct Holder
			{
				EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

				~Holder()
				{
					if (ctx)
						EVP_CIPHER_CTX_free(ctx);
				}
			};

			thread_local Holder h;

			// Reset or recreate the context (recreation of the ctx can happen if it failed on a previous attempt)
			if (!h.ctx)
				h.ctx = EVP_CIPHER_CTX_new();
			else
				EVP_CIPHER_CTX_reset(h.ctx);

			return h.ctx;
		}

		inline int Encrypt(unsigned char* plaintext, int plaintext_len,
			unsigned char* aad, int aad_len,
			unsigned char* key,
			unsigned char* iv, int iv_len,
			unsigned char* ciphertext,
			unsigned char* tag)
		{
			int len;
			int ciphertext_len;


			// Acquire the thread's context, ready for a new operation
			EVP_CIPHER_CTX* ctx = AcquireThreadCtx();
			if (!ctx)
				return -1;

			/* Initialise the encryption operation. */
			if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL))
				return -1;

			/*
			 * Set IV length if default 12 bytes (96 bits) is not appropriate
			 */
			if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL))
				return -3;

			/* Initialise key and IV */
			if (1 != EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv))
				return -4;

			/*
			 * Provide any AAD data. This can be called zero or more times as
			 * required
			 */
			if (1 != EVP_EncryptUpdate(ctx, NULL, &len, aad, aad_len))
				return -5;

			/*
			 * Provide the message to be encrypted, and obtain the encrypted output.
			 * EVP_EncryptUpdate can be called multiple times if necessary
			 */
			if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
				return -6;
			ciphertext_len = len;

			/*
			 * Finalise the encryption. Normally ciphertext bytes may be written at
			 * this stage, but this does not occur in GCM mode
			 */
			if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
				return -7;
			ciphertext_len += len;

			/* Get the tag */
			if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag))
				return -8;

			return ciphertext_len;
		}

		inline int Decrypt(unsigned char* ciphertext, int ciphertext_len,
			unsigned char* aad, int aad_len,
			unsigned char* tag,
			unsigned char* key,
			unsigned char* iv, int iv_len,
			unsigned char* plaintext)
		{
			int len;
			int plaintext_len;
			int ret;

			// Acquire the thread's context, ready for a new operation
			EVP_CIPHER_CTX* ctx = AcquireThreadCtx();
			if (!ctx)
				return -1;

			/* Initialise the decryption operation. */
			if (!EVP_DecryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL))
				return -2;

			/* Set IV length. Not necessary if this is 12 bytes (96 bits) */
			if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL))
				return -3;

			/* Initialise key and IV */
			if (!EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv))
				return -4;

			/*
			 * Provide any AAD data. This can be called zero or more times as
			 * required
			 */
			if (!EVP_DecryptUpdate(ctx, NULL, &len, aad, aad_len))
				return -5;

			/*
			 * Provide the message to be decrypted, and obtain the plaintext output.
			 * EVP_DecryptUpdate can be called multiple times if necessary
			 */
			if (!EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
				return -6;
			plaintext_len = len;

			/* Set expected tag value. Works in OpenSSL 1.0.1d and later */
			if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag))
				return -7;

			/*
			 * Finalise the decryption. A positive return value indicates success,
			 * anything else is a failure - the plaintext is not trustworthy.
			 */
			ret = EVP_DecryptFinal_ex(ctx, plaintext + len, &len);

			if (ret > 0)
			{
				/* Success */
				plaintext_len += len;
				return plaintext_len;
			}
			else
			{
				/* Verify failed */
				return -1;
			}
		}

	}
}
