#pragma once

#include <memory>
#include <string>
#include <map>
#include <pybind11/embed.h>

#if defined(_WIN32)
  #define HIDDEN
#else
  #define HIDDEN __attribute__((visibility("hidden")))
#endif

namespace py = pybind11;

// Forward declaration
class ModuleManager;
class Module;

class ModuleManager
{
    public:

        ModuleManager() {};

        bool LoadModule(const std::string &name, const py::dict &params);
        py::dict RunModule(const std::string &name, const py::dict &params);

    private:
        std::map<std::string, std::unique_ptr<Module>> m_mModules;
};

// compiler wont shut the fuck up about m_oModule being private for some reason,
// the hidden attribute fixes it
class HIDDEN Module
{
    friend ModuleManager;

    public:
        explicit Module(std::string name):
            m_sName(name){}

        // Move semantics
        Module(Module&&) noexcept = default;
        Module& operator=(Module&&) noexcept = default;

        // Non-copyable
        Module(const Module&) = delete;
        Module& operator=(const Module&) = delete;

    private:
        std::string m_sName;
        py::module m_oModule;

    private:
        py::dict Run(const std::string function, const py::dict &params);
};
