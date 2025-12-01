#include "ModuleManager.hpp"
#include <DatabaseInterface.hpp>
#include <stdexcept>

using DBAction = DatabaseInterface::Action;

static std::map<std::string, std::string> Run(ModuleManager &mm, py::dict &args)
{
    py::dict retArgs = mm.RunModule(DatabaseInterface::moduleName, args);
    std::map<std::string, std::string> result;
    
    for (auto item : retArgs) {
        std::string key   = py::str(item.first);
        std::string value = py::str(item.second);
        result[key] = value;
    }

    return result;
}

std::map<std::string, std::string> DatabaseInterface::Query(ModuleManager &mm, Action action) 
{
    if (action != DBAction::DELETEALL) {
        throw std::invalid_argument("Invalid query arguments");
    }

    py::dict runArgs;
    runArgs[py::str("action")] = py::str("delete_all");

    return Run(mm, runArgs);
}

std::map<std::string, std::string> DatabaseInterface::Query(ModuleManager &mm, Action action, const std::string &file)
{
    if (action != DBAction::SELECT && action != DBAction::DELETEONE) {
        throw std::invalid_argument("Invalid query arguments");
    }
    py::dict runArgs;
    if (action == DBAction::SELECT)
        runArgs[py::str("action")] = py::str("select");
    else
        runArgs[py::str("action")] = py::str("delete_one");
    
    runArgs[py::str("file")] = py::str(file);

    return Run(mm, runArgs);
}

std::map<std::string, std::string> DatabaseInterface::Query(ModuleManager &mm, Action action, const std::string &file, const std::string &hash)
{
    if (action != DBAction::INSERT) {
        throw std::invalid_argument("Invalid query arguments");
    }

    py::dict runArgs;
    runArgs[py::str("action")] = py::str("insert");
    runArgs[py::str("file")] = py::str(file);
    runArgs[py::str("hash")] = py::str(hash);

    return Run(mm, runArgs);
}

