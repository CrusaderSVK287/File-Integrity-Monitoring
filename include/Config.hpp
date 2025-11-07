#pragma once
#include <yaml-cpp/yaml.h>
#include <string>
#include <iostream>

class Config {
public:
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    static Config& getInstance();

    template<typename T>
    T get(const std::string& keyPath) const;
    template<typename T>
    T get(const std::string& keyPath, const T& def) const;

    std::string FilePath() {return m_FilePath;}
    bool Initialize();

    static void __Dump() {std::cout << getInstance().config << std::endl;};

private:
    YAML::Node config;
    std::string m_FilePath = "";

    explicit Config();

    YAML::Node getNode(const std::string& keyPath) const;
};

template<typename T>
T Config::get(const std::string& keyPath) const {
    YAML::Node node = getNode(keyPath);
    return node.as<T>();
}

template<typename T>
T Config::get(const std::string& keyPath, const T& def) const {
    try {
        return get<T>(keyPath);
    } catch (const std::runtime_error&) {
        return def;
    }
}

