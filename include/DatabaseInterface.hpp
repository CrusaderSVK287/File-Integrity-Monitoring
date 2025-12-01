#pragma once

#include <ModuleManager.hpp>
#include <map>
#include <string>

// for convenience. its map of strings to strings
using DatabaseQuery = std::map<std::string, std::string>;

class DatabaseInterface {
public:
    enum class Action {
        SELECT,
        INSERT,
        DELETEONE,
        DELETEALL,
    };

public:
    // make the class virtual
    DatabaseInterface() = delete;

    static constexpr std::string moduleName = "db";

    static std::map<std::string, std::string> Query(ModuleManager &mm, Action action);
    static std::map<std::string, std::string> Query(ModuleManager &mm, Action action, const std::string &file);
    static std::map<std::string, std::string> Query(ModuleManager &mm, Action action, const std::string &file, const std::string &hash);
};
