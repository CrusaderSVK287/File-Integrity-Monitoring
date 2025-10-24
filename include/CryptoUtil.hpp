#pragma once

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>

#include <string>
#include <vector>

class SHA256Util
{
    public:
        static std::string sha256(const std::string &input);
};

class PBKDF2Util {
public:
    // Generate a cryptographically secure random salt (default 16 bytes)
    static std::vector<unsigned char> GenerateSalt(size_t length = 16);

    // Derive a key using PBKDF2-HMAC-SHA256
    static std::vector<unsigned char> DeriveKey(const std::string& password,
                                                 const std::vector<unsigned char>& salt,
                                                 int iterations = 100000,
                                                 size_t dk_len = 32);

    // Verify a password against a stored salt and derived key
    static bool VerifyPassword(const std::string& password,
                                const std::vector<unsigned char>& salt,
                                const std::vector<unsigned char>& stored_dk,
                                int iterations = 100000);

    // Helper: Convert binary data to lowercase hex string
    static std::string ToHex(const unsigned char* data, size_t len);

    // Helper: Convert hex string to binary vector
    static std::vector<unsigned char> FromHex(const std::string& hex);
};
