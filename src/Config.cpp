#include <Log.hpp>
#include <Config.hpp>
#include <stdexcept>
#include <yaml-cpp/node/emit.h>
#include <yaml-cpp/node/node.h>

Config& Config::getInstance(const std::string& filename) {
    static Config instance(filename);
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

