#include <Log.hpp>
#include <Config.hpp>
#include <cstdint>
#include <stdexcept>
#include <yaml-cpp/exceptions.h>
#include <yaml-cpp/node/emit.h>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/node/parse.h>
#include <SecurityManager.hpp>
#include <CryptoUtil.hpp>

#include <fstream>
#include <string>
#include <iostream>

static bool isValidYaml(const std::string& text)
{
    try {
        YAML::Node node = YAML::Load(text);
        return true;  // parsed successfully
    } catch (const YAML::ParserException& e) {
        // syntax error, invalid YAML
        return false;
    } catch (const YAML::Exception& e) {
        // other YAML-related exception
        return false;
    }
}

static std::string LoadFile(const std::string& path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file) return {};
    return std::string((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
}

Config::Config()
{
    //TODO: Add proper path and windows path
    m_FilePath = "config.yaml";
}

Config& Config::getInstance() {
    static Config instance;
    return instance;
}

YAML::Node Config::getNode(const std::string& keyPath) const {
    logging::info("Getting configuration for " + keyPath);
    YAML::Node node = YAML::Clone(config);

    size_t start = 0, end;
    std::string key;

    while ((end = keyPath.find('.', start)) != std::string::npos) {
        key = keyPath.substr(start, end - start);

        YAML::Node next = node[key];
        if (!next.IsDefined())
            throw std::runtime_error("Invalid key in path: " + keyPath);

        node = next;
        start = end + 1;
    }

    key = keyPath.substr(start);
    YAML::Node finalNode = node[key];
    if (!finalNode.IsDefined())
        throw std::runtime_error("Key path not found: " + keyPath);

    return finalNode;
}

bool Config::Initialize()
{
    std::string content = LoadFile(m_FilePath);
    bool isHex = !content.empty() && 
        std::all_of(content.begin(), content.end(), [](unsigned char c) {
            return std::isxdigit(c);
        });

    try {
        if (isHex) {
            SecurityManager &SecurityMgr = SecurityManager::getInstance();

            std::string pwd = SecurityManager::GetPassword();
            if (!SecurityMgr.VerifyPassword(pwd)) {
                return 1;
            }

            std::string csalt = SecurityMgr.GetCryptoSalt();
            uint32_t iterations = SecurityMgr.GetIterations();
            std::vector<unsigned char> key_raw = PBKDF2Util::DeriveKey(pwd, csalt, iterations);
            pwd.erase(); // Erase sensitive data
            std::string key = PBKDF2Util::ToHex(key_raw.data(), key_raw.size());

            std::string iv = SecurityMgr.GetIV();
            std::string tag = SecurityMgr.GetTag();

            logging::msg("Decrypting configuration file");
            // override content
            content = AESUtil::AESGcmDecrypt(key, 32, content, iv, tag);

            // While we are here, set up the LogKey
            std::string lsalt = SecurityMgr.GetLogSalt();
            std::vector<unsigned char> LogKey = PBKDF2Util::DeriveKey(pwd, csalt, iterations);
            SecurityMgr.SetLogKey(PBKDF2Util::ToHex(LogKey.data(), LogKey.size()));
        } else if (!isValidYaml(content)) {
            logging::err("The configuration file appears to be corrupted");
            return false;
        }
    } catch (const std::runtime_error &e) {
        logging::err(e.what());
        return false;
    }

    config = YAML::Load(content);
    
    return true;
}

