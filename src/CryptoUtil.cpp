#include <CryptoUtil.hpp>
#include <openssl/evp.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

std::string SHA256Util::sha256(const std::string& input)
{
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx)
        throw std::runtime_error("Failed to create EVP_MD_CTX");

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1 ||
        EVP_DigestUpdate(ctx, input.data(), input.size()) != 1 ||
        EVP_DigestFinal_ex(ctx, hash, &hash_len) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("SHA-256 computation failed using EVP API");
    }

    EVP_MD_CTX_free(ctx);

    std::ostringstream oss;
    for (unsigned int i = 0; i < hash_len; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(hash[i]);
    }

    return oss.str();
}

std::string PBKDF2Util::ToHex(const unsigned char* data, size_t len)
{
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; ++i) {
        oss << std::setw(2) << static_cast<int>(data[i]);
    }
    return oss.str();
}

std::vector<unsigned char> PBKDF2Util::FromHex(const std::string& hex)
{
    if (hex.size() % 2 != 0)
        throw std::runtime_error("Invalid hex string length");

    std::vector<unsigned char> out;
    out.reserve(hex.size() / 2);

    auto hex_val = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
        if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
        return -1;
    };

    for (size_t i = 0; i < hex.size(); i += 2) {
        int hi = hex_val(hex[i]);
        int lo = hex_val(hex[i + 1]);
        if (hi < 0 || lo < 0)
            throw std::runtime_error("Invalid hex character");
        out.push_back(static_cast<unsigned char>((hi << 4) | lo));
    }

    return out;
}

// ----- PBKDF2 functionality ---------------------------------------------------

std::vector<unsigned char> PBKDF2Util::GenerateSalt(size_t length)
{
    if (length == 0)
        throw std::invalid_argument("Salt length must be > 0");

    std::vector<unsigned char> salt(length);
    if (RAND_bytes(salt.data(), static_cast<int>(length)) != 1) {
        throw std::runtime_error("RAND_bytes failed to generate salt");
    }
    return salt;
}

std::vector<unsigned char> PBKDF2Util::DeriveKey(const std::string& password,
                                                        const std::vector<unsigned char>& salt,
                                                        int iterations,
                                                        size_t dk_len)
{
    if (iterations <= 0)
        throw std::invalid_argument("Iterations must be > 0");
    if (dk_len == 0)
        throw std::invalid_argument("Derived key length must be > 0");

    std::vector<unsigned char> dk(dk_len);

    if (PKCS5_PBKDF2_HMAC(password.data(),
                          static_cast<int>(password.size()),
                          salt.data(),
                          static_cast<int>(salt.size()),
                          iterations,
                          EVP_sha256(),
                          static_cast<int>(dk_len),
                          dk.data()) != 1) {
        throw std::runtime_error("PBKDF2-HMAC-SHA256 failed");
    }

    return dk;
}

bool PBKDF2Util::VerifyPassword(const std::string& password,
                                       const std::vector<unsigned char>& salt,
                                       const std::vector<unsigned char>& stored_dk,
                                       int iterations)
{
    std::vector<unsigned char> dk = DeriveKey(password, salt, iterations, stored_dk.size());
    return CRYPTO_memcmp(dk.data(), stored_dk.data(), dk.size()) == 0;
}
