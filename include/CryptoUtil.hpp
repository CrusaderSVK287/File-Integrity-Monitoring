#pragma once

#include "Filters.hpp"
#include <cstdint>
#include <memory>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>

#include <string>
#include <tuple>
#include <vector>
#include <map>
#include <unordered_set>

using FilterMap = std::map<std::string, std::vector<std::unique_ptr<Filter>>>;

class SHAFileUtil
{
public:
    static std::string SHA256(
        const std::string &filename,
        const FilterMap &filters);

    static std::string SHA512(
        const std::string &filename,
        const FilterMap &filters = GetEmptyFilterMap()
    );

    static std::string Blake2s256(
        const std::string &filename,
        const FilterMap &filters = GetEmptyFilterMap()
    );

    static std::string Blake2s512(
        const std::string &filename,
        const FilterMap &filters = GetEmptyFilterMap()
    );

    static std::string SHA3_256(
        const std::string &filename,
        const FilterMap &filters = GetEmptyFilterMap()
    );

    static std::string SHA3_512(
        const std::string &filename,
        const FilterMap &filters = GetEmptyFilterMap()
    );

    static std::string SHA_Agnostic(
        const std::string &filename,
        const EVP_MD *a,
        const FilterMap &filters = GetEmptyFilterMap()
    );

private:
    static const FilterMap& GetEmptyFilterMap()
    {
        static const FilterMap empty;
        return empty;
    };
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

    static std::vector<unsigned char> DeriveKey(const std::string& password,
                                                        const std::string& salt,
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

class AESUtil {
    public:
        static std::string GenerateIV(size_t length);
        static std::string ToHex(const std::vector<unsigned char>& data);

        static std::tuple<std::string, std::string> AESGcmEncrypt(
                    const std::string &key, const int key_len,
                    const std::string &plaintext,
                    const std::string &iv, const int iv_len,
                    const int tag_len = 16);
        static std::string AESGcmDecrypt(const std::string &key, const int key_len,
                     const std::string &ciphertext,
                     const std::string &iv,
                     const std::string &tag, const int tag_len = 16);
};
