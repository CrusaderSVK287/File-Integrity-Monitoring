#include <CryptoUtil.hpp>
#include <Log.hpp>
#include <cstdint>
#include <cstdlib>
#include <openssl/evp.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <tuple>

// Helper: convert a single hex char to value (throws on invalid)
static unsigned char hexCharToVal(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    throw std::invalid_argument("Invalid hex character");
}

// Helper: convert hex string to bytes (returns vector)
static std::vector<unsigned char> hexStringToBytes(const std::string &hex) {
    if (hex.size() % 2 != 0)
        throw std::invalid_argument("Hex string must have even length");

    std::vector<unsigned char> out;
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        unsigned char high = hexCharToVal(hex[i]);
        unsigned char low  = hexCharToVal(hex[i + 1]);
        out.push_back(static_cast<unsigned char>((high << 4) | low));
    }
    return out;
}

// Helper: convert bytes to lowercase hex string
static std::string bytesToHex(const unsigned char *data, size_t len) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; ++i)
        oss << std::setw(2) << static_cast<int>(data[i]);
    return oss.str();
}

static std::string bytesToHex(const std::vector<unsigned char> &v) {
    return bytesToHex(v.data(), v.size());
}

std::string SHAFileUtil::SHA_Agnostic(const std::string& path, const EVP_MD* algorithm ,const FilterMap &filters)
{
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx)
        throw std::runtime_error("Failed to create EVP_MD_CTX");

    if (EVP_DigestInit_ex(ctx, algorithm, nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Digest initialization failed");
    }

    std::ifstream file(path, std::ios::binary);
    if (!file)
        throw std::runtime_error("Failed to open file: " + path);

    uint64_t lineNumber = 0;
    std::string line;
    auto it = filters.find(path);
    // just for logging purposes
    if (it != filters.end()) 
        logging::info("Filter for " + path + " found, skiping filtered lines");

    while (std::getline(file, line)) {
        ++lineNumber;


        if (it != filters.end()) {
            const auto& excludedLines = it->second;
            if (excludedLines.contains(lineNumber))
                continue; // Skip this line
        }

        // Include newline so the hash matches file structure
        line.push_back('\n');
        EVP_DigestUpdate(ctx, line.data(), line.size());
    }
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;

    if (EVP_DigestFinal_ex(ctx, hash, &hash_len) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Digest finalization failed");
    }

    EVP_MD_CTX_free(ctx);

    std::ostringstream oss;
    for (unsigned int i = 0; i < hash_len; ++i)
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);

    return oss.str();
}

std::string SHAFileUtil::SHA256(const std::string& input ,const FilterMap &filters)
{
    logging::info("Running sha256 calculation");
    return SHAFileUtil::SHA_Agnostic(input, EVP_sha256(), filters);
}

std::string SHAFileUtil::SHA512(const std::string& input ,const FilterMap &filters)
{
    logging::info("Running sha512 calculation");
    return SHAFileUtil::SHA_Agnostic(input, EVP_sha512(), filters);
}
std::string SHAFileUtil::Blake2s256(const std::string &input ,const FilterMap &filters)
{
    logging::info("Running blake2s256 calculation");
    return SHAFileUtil::SHA_Agnostic(input, EVP_sha512(), filters);
}
std::string SHAFileUtil::Blake2s512(const std::string &input ,const FilterMap &filters)
{
    logging::info("Running blake2s512 calculation");
    return SHAFileUtil::SHA_Agnostic(input, EVP_blake2b512(), filters);
}
std::string SHAFileUtil::SHA3_256(const std::string& input ,const FilterMap &filters)
{
    logging::info("Running sha256 calculation");
    return SHAFileUtil::SHA_Agnostic(input, EVP_sha3_256(), filters);
}

std::string SHAFileUtil::SHA3_512(const std::string& input ,const FilterMap &filters)
{
    logging::info("Running sha512 calculation");
    return SHAFileUtil::SHA_Agnostic(input, EVP_sha3_512(), filters);
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
    logging::info("Generating salt");
    if (length == 0)
        throw std::invalid_argument("Salt length must be > 0");

    std::vector<unsigned char> salt(length);
    if (RAND_bytes(salt.data(), static_cast<int>(length)) != 1) {
        throw std::runtime_error("RAND_bytes failed to generate salt");
    }
    return salt;
}

std::vector<unsigned char> PBKDF2Util::DeriveKey(const std::string& password,
                                                        const std::string& salt,
                                                        int iterations,
                                                        size_t dk_len)
{
    return PBKDF2Util::DeriveKey(password, hexStringToBytes(salt), iterations, dk_len);
}

std::vector<unsigned char> PBKDF2Util::DeriveKey(const std::string& password,
                                                        const std::vector<unsigned char>& salt,
                                                        int iterations,
                                                        size_t dk_len)
{
    logging::info("Deriving key");
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
    logging::info("Verifing password");
    std::vector<unsigned char> dk = DeriveKey(password, salt, iterations, stored_dk.size());
    return CRYPTO_memcmp(dk.data(), stored_dk.data(), dk.size()) == 0;
}

std::string AESUtil::GenerateIV(size_t length = 12) {
    if (length == 0) throw std::invalid_argument("IV length must be > 0");
    std::vector<unsigned char> iv(length);
    if (RAND_bytes(iv.data(), static_cast<int>(length)) != 1) {
        throw std::runtime_error("RAND_bytes failed to generate IV");
    }
    return ToHex(iv);
}

std::string AESUtil::ToHex(const std::vector<unsigned char>& data) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned char b : data)
        oss << std::setw(2) << static_cast<int>(b);
    return oss.str();
}

std::tuple<std::string, std::string> AESUtil::AESGcmEncrypt(
        const std::string &key_hex, const int key_len,
        const std::string &plaintext,
        const std::string &iv_hex, const int iv_len,
        const int tag_len)
{
    auto key = hexStringToBytes(key_hex);
    auto iv  = hexStringToBytes(iv_hex);

    if (static_cast<int>(key.size()) != key_len) {
        throw std::invalid_argument("Key length (bytes) does not match provided key_len");
    }
    if (static_cast<int>(iv.size()) != iv_len) {
        throw std::invalid_argument("IV length (bytes) does not match provided iv_len");
    }
    if (tag_len <= 0 || tag_len > 16) {
        throw std::invalid_argument("tag_len must be between 1 and 16");
    }

    // Select cipher
    const EVP_CIPHER *cipher = nullptr;
    if (key_len == 32) {
        cipher = EVP_aes_256_gcm();
    } else if (key_len == 16) {
        cipher = EVP_aes_128_gcm();
    } else {
        throw std::invalid_argument("Unsupported key_len; expected 16 or 32 (bytes)");
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("EVP_CIPHER_CTX_new failed");

    try {
        if (EVP_EncryptInit_ex(ctx, cipher, NULL, NULL, NULL) != 1)
            throw std::runtime_error("EVP_EncryptInit_ex (cipher init) failed");

        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL) != 1)
            throw std::runtime_error("EVP_CIPHER_CTX_ctrl (set IV len) failed");

        if (EVP_EncryptInit_ex(ctx, NULL, NULL, key.data(), iv.data()) != 1)
            throw std::runtime_error("EVP_EncryptInit_ex (set key/iv) failed");

        // Prepare output buffer (ciphertext same length as plaintext for GCM)
        std::vector<unsigned char> ciphertext(plaintext.size());
        int out_len = 0;
        int total_len = 0;

        if (!plaintext.empty()) {
            if (EVP_EncryptUpdate(ctx,
                                  ciphertext.data(),
                                  &out_len,
                                  reinterpret_cast<const unsigned char*>(plaintext.data()),
                                  static_cast<int>(plaintext.size())) != 1)
            {
                throw std::runtime_error("EVP_EncryptUpdate failed");
            }
            total_len = out_len;
        } else {
            total_len = 0;
        }

        // Finalize (GCM typically doesn't add ciphertext bytes, but call anyway)
        if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + total_len, &out_len) != 1)
            throw std::runtime_error("EVP_EncryptFinal_ex failed");

        total_len += out_len;
        ciphertext.resize(total_len);

        // Get authentication tag
        std::vector<unsigned char> tag_buf(tag_len);
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, tag_len, tag_buf.data()) != 1)
            throw std::runtime_error("EVP_CIPHER_CTX_ctrl (GET_TAG) failed");

        EVP_CIPHER_CTX_free(ctx);
        ctx = nullptr;

        return std::make_tuple(bytesToHex(ciphertext), bytesToHex(tag_buf));
    } catch (...) {
        if (ctx) EVP_CIPHER_CTX_free(ctx);
        throw; // rethrow
    }
}

std::string AESUtil::AESGcmDecrypt(
        const std::string &key_hex, const int key_len,
        const std::string &ciphertext_hex,
        const std::string &iv_hex,
        const std::string &tag_hex, const int tag_len)
{
    auto key = hexStringToBytes(key_hex);
    auto iv = hexStringToBytes(iv_hex);
    auto ciphertext = hexStringToBytes(ciphertext_hex);
    auto tag = hexStringToBytes(tag_hex);
    if (static_cast<int>(key.size()) != key_len)
        throw std::invalid_argument("Key length (bytes) does not match key_len");
    if (static_cast<int>(iv.size()) == 0)
        throw std::invalid_argument("IV is empty");
    if (tag.size() != static_cast<size_t>(tag_len))
        throw std::invalid_argument("Tag length mismatch");

    const EVP_CIPHER *cipher = nullptr;
    if (key_len == 32)
        cipher = EVP_aes_256_gcm();
    else if (key_len == 16)
        cipher = EVP_aes_128_gcm();
    else
        throw std::invalid_argument("Unsupported key_len; expected 16 or 32 bytes");

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("EVP_CIPHER_CTX_new failed");

    try {
        if (EVP_DecryptInit_ex(ctx, cipher, NULL, NULL, NULL) != 1)
            throw std::runtime_error("EVP_DecryptInit_ex (cipher init) failed");

        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(iv.size()), NULL) != 1)
            throw std::runtime_error("EVP_CIPHER_CTX_ctrl (set IV len) failed");

        if (EVP_DecryptInit_ex(ctx, NULL, NULL, key.data(), iv.data()) != 1)
            throw std::runtime_error("EVP_DecryptInit_ex (set key/iv) failed");

        std::vector<unsigned char> plaintext(ciphertext.size());
        int out_len = 0;
        int total_len = 0;

        if (!ciphertext.empty()) {
            if (EVP_DecryptUpdate(ctx,
                                  plaintext.data(),
                                  &out_len,
                                  ciphertext.data(),
                                  static_cast<int>(ciphertext.size())) != 1)
            {
                throw std::runtime_error("EVP_DecryptUpdate failed");
            }
            total_len = out_len;
        }

        // Set expected tag before finalizing
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, tag_len, const_cast<unsigned char*>(tag.data())) != 1)
            throw std::runtime_error("EVP_CIPHER_CTX_ctrl (SET_TAG) failed");

        int ret = EVP_DecryptFinal_ex(ctx, plaintext.data() + total_len, &out_len);
        if (ret <= 0) {
            throw std::runtime_error("Decryption failed: tag mismatch or corrupted data");
        }

        total_len += out_len;
        plaintext.resize(total_len);

        EVP_CIPHER_CTX_free(ctx);
        return std::string(reinterpret_cast<char*>(plaintext.data()), plaintext.size());
    } catch (...) {
        EVP_CIPHER_CTX_free(ctx);
        throw;
    }
}
