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
    std::string FilePath() {return m_FilePath;}

    static void __Dump() {std::cout << getInstance().config << std::endl;};

private:
    const YAML::Node config;
    const std::string m_FilePath = "";

    explicit Config(const std::string& filename) :
        config(YAML::LoadFile(filename)),
        m_FilePath(filename){};

    YAML::Node getNode(const std::string& keyPath) const;
};

template<typename T>
T Config::get(const std::string& keyPath) const {
    YAML::Node node = getNode(keyPath);
    return node.as<T>();
}

