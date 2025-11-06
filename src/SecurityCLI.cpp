#include <SecurityCLI.hpp>
#include <SecurityManager.hpp>
#include <Log.hpp>
#include <Config.hpp>
#include <CryptoUtil.hpp>
#include <cstdint>
#include <stdexcept>
#include <yaml-cpp/node/parse.h>
#include <yaml-cpp/yaml.h>

#include <fstream>
#include <string>
#include <iostream>

static const std::string _usage = "./monitor --security (encrypt/decrypt) (config/logs) | verify | help";

static int FailWithUsage() {
    logging::err("Incorrect usage. Please provide operation: " + _usage);
    return 1;
}

static std::string LoadFile(const std::string& path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file) return {};
    return std::string((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
}

int SecurityCLI::Enter(const int argc, const char **argv)
{
    if (!SecurityManager::IsAdministrator()) {
#ifdef _WIN32
        std::cerr << "You must run this program as Administrator." << std::endl;
#else
        std::cerr << "You must run this program with sudo or as root." << std::endl;
#endif
        return -1;
    }

    if (argc < 3)
        return FailWithUsage();

    std::string op = argv[2];

    if (op == "usage" || op == "help") {
        return Usage();
    } else if (op == "encrypt") {
        if (argc != 4) return FailWithUsage();
        std::string sub = argv[3];
        if (sub == "config") 
            return EncryptConfig();
        else
             return FailWithUsage();
    } else if (op == "decrypt") {
        if (argc != 4) return FailWithUsage();
        std::string sub = argv[3];
        if (sub == "config") 
            return DecryptConfig();
        else if (sub == "logs") 
            return DecryptLogs();
        else
             return FailWithUsage();
    } else if (op == "verify") {
        return Verify();
    } else {
        return FailWithUsage();
    }

    return 0;
}

int SecurityCLI::EncryptConfig() {

    SecurityManager &SecurityMgr = SecurityManager::getInstance();
    Config &Cfg = Config::getInstance();
    std::string pwd = SecurityManager::GetPassword();
    if (!SecurityMgr.VerifyPassword(pwd)) {
        return 1;
    }

    try {
        std::string csalt = SecurityMgr.GetCryptoSalt();
        uint32_t iterations = SecurityMgr.GetIterations();
        std::vector<unsigned char> key_raw =  PBKDF2Util::DeriveKey(pwd, csalt, iterations);
        pwd.erase(); // No longer needed, so erase to avoid possible core dump attack 
        std::string key = PBKDF2Util::ToHex(key_raw.data(), key_raw.size());
    
        std::string plaintext = LoadFile(Cfg.FilePath());
    
        // std::string, std::string
        auto [ciphertext, tag] = AESUtil::AESGcmEncrypt(key, 32, plaintext, SecurityMgr.GetIV(), 12);
        plaintext.clear(); // No longer needed, so erase to avoid possible core dump attack
    
        if (!SecurityMgr.SetTag(tag)) {
            //TODO:: some log idk
            return 1;
        }
    
        std::ofstream fout(Cfg.FilePath());
        fout << ciphertext;
    
    } catch (const std::runtime_error &e) {
        logging::err(e.what());
    }
    return 0;
}

int SecurityCLI::DecryptConfig() {

    SecurityManager &SecurityMgr = SecurityManager::getInstance();
    Config &Cfg = Config::getInstance();

    try {
        std::string pwd = SecurityManager::GetPassword();
        if (!SecurityMgr.VerifyPassword(pwd)) {
            return 1;
        }

        std::string csalt = SecurityMgr.GetCryptoSalt();
        uint32_t iterations = SecurityMgr.GetIterations();
        std::vector<unsigned char> key_raw = PBKDF2Util::DeriveKey(pwd, csalt, iterations);
        pwd.erase(); // Erase sensitive data
        std::string key = PBKDF2Util::ToHex(key_raw.data(), key_raw.size());

        std::string ciphertext = LoadFile(Cfg.FilePath());

        std::string iv = SecurityMgr.GetIV();
        std::string tag = SecurityMgr.GetTag();

        std::string plaintext = AESUtil::AESGcmDecrypt(key, 32, ciphertext, iv, tag);

        ciphertext.clear(); // Wipe sensitive buffer

        std::ofstream fout(Cfg.FilePath());
        fout << plaintext;

        plaintext.clear(); // Wipe plaintext from memory
    } catch (const std::runtime_error &e) {
        logging::err(e.what());
    }

    return 0;
}

int SecurityCLI::DecryptLogs() {
    return 0;
}

int SecurityCLI::Verify() {
    return 0;
}

int SecurityCLI::Usage() {
    return 0;
}
