#include <ModuleManager.hpp>

#include <pybind11/embed.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <map>

namespace py = pybind11;

bool ModuleManager::LoadModule(const std::string &name, const py::dict &params)
{
    auto module = std::make_unique<Module>(name);

    try {
        module->m_oModule = py::module::import(name.c_str());
    } catch (const py::error_already_set& e) {
        std::cerr << "Failed to import Python module '" << name << "':\n"
                  << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "[ModuleManager] Unexpected exception while importing '"
                  << name << "': " << e.what() << std::endl;
        return false;
    }

    // Initialise the module
    py::dict init_result = module->Run("init", params);
    std::string status = init_result["status"].cast<std::string>();
    if (status.compare("OK")) {
        std::cerr << "Error loading module " << name << ":\n" << status << std::endl;
        return false;
    }

    m_mModules.emplace(name, std::move(module));

    // TODO: add logger
    std::cout << "[ModuleManager] Successfully loaded module '" << name << "'.\n";
    return true;
}

py::dict ModuleManager::RunModule(const std::string &name, const py::dict &params)
{
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

