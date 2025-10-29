#pragma once
#include <yaml-cpp/yaml.h>
#include <string>
#include <iostream>

class Config {
public:
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    static Config& getInstance(const std::string& filename = "config.yaml");

    template<typename T>
    T get(const std::string& keyPath) const;
    
    static void __Dump() {std::cout << getInstance().config << std::endl;};

private:
    YAML::Node config;

    explicit Config(const std::string& filename);

    YAML::Node getNode(const std::string& keyPath) const;
};

template<typename T>
T Config::get(const std::string& keyPath) const {
    YAML::Node node = getNode(keyPath);
    return node.as<T>();
}

