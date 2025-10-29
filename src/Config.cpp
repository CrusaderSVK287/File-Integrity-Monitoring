#include <log.hpp>
#include <Config.hpp>
#include <stdexcept>

Config& Config::getInstance(const std::string& filename) {
    static Config instance(filename);
    return instance;
}

Config::Config(const std::string& filename) {
    config = YAML::LoadFile(filename);
}

YAML::Node Config::getNode(const std::string& keyPath) const {
    logging::info("Getting configuration for " + keyPath);
    YAML::Node node = config;
    size_t start = 0, end;
    std::string key;

    while ((end = keyPath.find('.', start)) != std::string::npos) {
        key = keyPath.substr(start, end - start);
        node = node[key];
        start = end + 1;
    }

    key = keyPath.substr(start);
    node = node[key];

    if (!node) {
        throw std::runtime_error("Key path not found: " + keyPath);
    }

    return node;
}

