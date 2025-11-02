#include <ModuleManager.hpp>
#include <Log.hpp>

#include <pybind11/embed.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <map>

namespace py = pybind11;

bool ModuleManager::LoadModule(const std::string &name, const py::dict &params)
{
    logging::msg("Loading python module - " + name);

    auto module = std::make_unique<Module>(name);

    try {
        module->m_oModule = py::module::import(name.c_str());
    } catch (const py::error_already_set& e) {
        logging::err("[Pyerror] Failed to import Python module '" + name + "': " + e.what());
        return false;
    } catch (const std::exception& e) {
        logging::err("[ModuleManager] Unexpected exception while importing '" + name + "': " + e.what());
        return false;
    }

    // Initialise the module
    py::dict init_result = module->Run("init", params);
    std::string status = init_result["status"].cast<std::string>();
    if (status.compare("OK")) {
        logging::err("Error loading module " + name + ": " + status);
        return false;
    }

    m_mModules.emplace(name, std::move(module));

    logging::msg("[ModuleManager] Successfully loaded python module " + name + ". Return value: " + status);
    return true;
}

py::dict ModuleManager::RunModule(const std::string &name, const py::dict &params)
{
    logging::msg("[ModuleManager] Running python module " + name);
    auto it = m_mModules.find(name);
    if (it == m_mModules.end()) {
        throw std::runtime_error("Module '" + name + "' not found, cannot run");
    }

    return m_mModules[name]->Run("run", params);
}

py::dict Module::Run(const std::string function, const py::dict &params)
{
    py::dict result;

    try {
        result = m_oModule.attr(function.c_str())(params).cast<py::dict>();    
    } catch (py::error_already_set &e) {
        result.clear();
        result["status"] = "error";
        result["message"] = e.what();
    }

    return result;
}

