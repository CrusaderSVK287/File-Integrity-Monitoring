#include <SecurityCLI.hpp>
#include <SecurityManager.hpp>
#include <Log.hpp>
#include <Config.hpp>
#include <CryptoUtil.hpp>
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <yaml-cpp/node/parse.h>
#include <yaml-cpp/yaml.h>
#include <PortabilityUtils.hpp>

#include <fstream>
#include <string>
#include <iostream>
#include <vector>

namespace fs = std::filesystem;

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
    } else if (op == "pwd") {
        return GeneratePassword();
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
        auto [ciphertext, tag] = AESUtil::AESGcmEncrypt(key, 32, plaintext, SecurityMgr.GetCIV(), 12);
        plaintext.clear(); // No longer needed, so erase to avoid possible core dump attack
    
        if (!SecurityMgr.SetTag(tag)) {
            logging::info("New configuration Tag was saved");
            return 1;
        }
    
        std::ofstream fout(Cfg.FilePath());
        fout << ciphertext;
    
    } catch (const std::runtime_error &e) {
        logging::err(e.what());
    }
    
    std::cout << "Configuration encrypted" << std::endl;
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
        std::string plaintext = AESUtil::AESGcmDecrypt(key, 32, ciphertext, SecurityMgr.GetCIV(), SecurityMgr.GetTag());
        ciphertext.clear(); // Wipe sensitive buffer

        std::ofstream fout(Cfg.FilePath(), std::ios::out | std::ios::binary);
        fout << plaintext;

        plaintext.clear(); // Wipe plaintext from memory
    } catch (const std::runtime_error &e) {
        logging::err(e.what());
    }

    std::cout << "Configuration decrypted" << std::endl;
    return 0;
}

static std::tuple<std::string, std::string> SplitAESTagAndData(const std::string& hexString, const int tagLenght = 32) {
    if (hexString.size() < tagLenght) {
        throw std::invalid_argument("Input string too short to contain AES tag");
    }

    std::string tag = hexString.substr(0, tagLenght);
    std::string data = hexString.substr(tagLenght);

    return std::make_tuple(tag, data);
}

std::vector<std::string> LoadFileLines(const std::string& inputPath) {
    std::vector<std::string> lines;
    std::ifstream file(inputPath);

    if (!file) {
        std::cerr << "Failed to open file: " << inputPath << std::endl;
        return lines;
    }

    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }

    return lines;
}

int SecurityCLI::DecryptLogs() {
    std::string dir = logging::LogDir();
    std::vector<std::string> files;

    try {
        if (!fs::exists(dir) || !fs::is_directory(dir)) {
            std::cerr << "Invalid log directory: " << dir << std::endl;
            return 1;
        }

        for (const auto& entry : fs::directory_iterator(dir)) {
            if (fs::is_regular_file(entry)) {
                files.push_back(entry.path().filename().string());
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
        return 1;
    }

#ifdef _WIN32
    std::string outDir = dir + "\\decrypted";
#else
    std::string outDir = dir + "/decrypted";
#endif

    fs::create_directories(outDir);

    SecurityManager &SecurityMgr = SecurityManager::getInstance();
    Config &Cfg = Config::getInstance();

    std::string pwd = SecurityManager::GetPassword();
    if (!SecurityMgr.VerifyPassword(pwd)) {
            return 1;
    }

    std::string lsalt = SecurityMgr.GetLogSalt();
    uint32_t iterations = SecurityMgr.GetIterations();
    std::vector<unsigned char> key_raw = PBKDF2Util::DeriveKey(pwd, lsalt, iterations);
    pwd.clear();
    std::string key = PBKDF2Util::ToHex(key_raw.data(), key_raw.size());
    std::string iv = SecurityMgr.GetLIV();

    for (const auto& filename : files) {
#ifdef _WIN32
        std::string inputPath = dir + "\\" + filename;
        std::string outputPath = outDir + "\\" + filename;
#else
        std::string inputPath = dir + "/" + filename;
        std::string outputPath = outDir + "/" + filename;
#endif

        std::vector<std::string> lines = LoadFileLines(inputPath);
        // We only want to decrypt the encrypted logs. Verify that this log 
        // is encrypted by checking whether all lines are hexstrings.
        // If even one of them isnt, dont decrypt the log as its eighter
        // not encrypted or corrupted
        bool isHex = true;
        for (auto &l : lines) {
            isHex = !l.empty() && 
            std::all_of(l.begin(), l.end(), [](unsigned char c) {
                return std::isxdigit(c);
            });

            // exit the foreach
            if (!isHex)
                break;
        }
        // continue with next logfile if this one is not encrypted/is corrupted
        if (!isHex)
            continue;


        std::ofstream outputFile(outputPath, std::ios::binary);
        if (!outputFile) {
            std::cerr << "Failed to open output file: " << outputPath << std::endl;
            continue;
        }

        for (const std::string &line : lines) {
            // Decrypt the data (replace this with your decryption function)
            auto [tag, data] = SplitAESTagAndData(line);
            std::string plaintext = AESUtil::AESGcmDecrypt(key, 32, data, iv, tag);
            outputFile << plaintext << std::endl;
        }
        outputFile.close();
    }

    return 0;
}

int SecurityCLI::Usage() {
    return 0;
}

int SecurityCLI::GeneratePassword() {
    SecurityManager &SecMgr = SecurityManager::getInstance();
    
    if (std::filesystem::exists(SecMgr.GetPwdFilePath()) && SecMgr.VerifyPassword()) {
        std::cout << std::endl;
        SecMgr.GenerateNewPasswordUserInput();
    } else if (!std::filesystem::exists(SecMgr.GetPwdFilePath())) {
        SecMgr.GenerateNewPasswordUserInput();
    } else {
        SLEEP(3);
        std::cout <<std::endl << "Failed to verify password" << std::endl;
    }

    return 0;
}
