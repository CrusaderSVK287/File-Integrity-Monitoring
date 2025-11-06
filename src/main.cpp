#include <Monitor.hpp>
#include <ModuleManager.hpp>
#include <SecurityManager.hpp>
#include <Config.hpp>
#include <Log.hpp>
#include <SecurityCLI.hpp>

#include <cstring>
#include <pybind11/embed.h>
#include <string>
#include <iostream>


namespace py = pybind11;

// Function strictly for testing modules, wont be used in actually code.
// Do what you wish here. When testing actuall app, dont forget to remove
// argv:
//  argv[1] - tested modules name
//  argv[2] - init key 1
//  argv[3] - init value 1
//  argv[4] - init key 2
//  argv[5] - init value 2
//  argv[x] - "-"
//  argv[x+1] - run key 1
//  argv[x+2] - run value 1
//  ....
static void _module_testing(int argc, const char **argv)
{
    const std::string moduleName = argv[2];

    py::dict initArgs;
    py::dict runArgs;

    bool parsingInit = true;

    for (int i = 3; i < argc; i += 2) {
        const std::string current = argv[i];

        if (current == "-") {
            parsingInit = false;
            i--; // adjust index because "-" has no paired value
            continue;
        }

        if (i + 1 >= argc) {
            std::cerr << "[_module_testing] Warning: Ignoring key without value: "
                      << argv[i] << "\n";
            break;
        }

        const std::string key = argv[i];
        const std::string value = argv[i + 1];

        if (parsingInit) {
            initArgs[py::str(key)] = py::str(value);
        } else {
            runArgs[py::str(key)] = py::str(value);
        }
    }

    // Output the parsed results for verification
    std::cout << "Module Name: " << moduleName << "\n";
    std::cout << "Init Args: " << py::str(initArgs).cast<std::string>() << "\n";
    std::cout << "Run Args: "  << py::str(runArgs).cast<std::string>() << "\n" 
        << "---------------------- INIT ----------------------" << std::endl;


    ModuleManager manager;
    if (!manager.LoadModule(moduleName, initArgs)) {
        std::cerr << "Failed to load module" << std::endl;
        return;
    }

    std::cout << "--------------------- RUNNING --------------------" << std::endl;
    py::dict runResult = manager.RunModule(moduleName, runArgs);
        
    std::cout << "{\n";
    for (auto item : runResult) {
        std::string key   = py::str(item.first);
        std::string value = py::str(item.second);
        std::cout << "  " << key << ": " << value << "\n";
    }
    std::cout << "}\n";
}

int main(int argc, const char **argv)
{
    // TODO: add config and method to change log verbosity
    logging::init(logging::LogVerbosity::highest);
    logging::info("Logger initialised");

    // Access the security CLI if needed
    if (argc > 1 && !strcmp(argv[1], "--security")) {
        logging::msg("Running application in simplified mode. Reason: Security handling");
        SecurityCLI cli;
        return cli.Enter(argc, argv);
    }

    py::scoped_interpreter guard{};
    logging::info("Python scoped interpreter pybind11 initialised");

    /* For testing purposes only, will delete later */
    if (argc > 1 && !strcmp(argv[1], "--module")) {
        logging::warn("[MODULE TESTER]: Running module tester. "
                "Module will be loaded and run. Then the program will exit");
        _module_testing(argc, argv);
        return 100;
    }

    // Add proper config file path
    Config &cfg = Config::getInstance();
    if (!cfg.Initialize())
        return 1;

    logging::msg("Configuration version: " + cfg.get<std::string>("version"));

    // TODO: startmonitoring will throw exceptions (check declaration), support that
    Monitor monitor;
    if (monitor.Initialise()) {
        monitor.StartMonitoring();
    }
    

    logging::msg("Exiting properly");
    return 0;
}

