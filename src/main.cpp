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

#include <CryptoUtil.hpp>
static int AES_test()
{
        try {
        // Example AES-256 key (32 bytes = 64 hex chars)
        std::string key_hex = "603deb1015ca71be2b73aef0857d7781"
                              "1f352c073b6108d72d9810a30914dff4";

        // Example 12-byte IV (24 hex chars)
        std::string iv_hex = "cafebabefacedbaddecaf888";

        // Example plaintext
        std::string plaintext = "The quick brown fox jumps over the lazy dog";

        // Encrypt
        auto [ciphertext_hex, tag_hex] = AESUtil::AESGcmEncrypt(
            key_hex, 32, plaintext, iv_hex, 12, 16);

        std::cout << "Plaintext:    " << plaintext << "\n";
        std::cout << "Ciphertext:   " << ciphertext_hex << "\n";
        std::cout << "Tag:          " << tag_hex << "\n";

        // Decrypt
        std::string decrypted = AESUtil::AESGcmDecrypt(
            key_hex, 32, ciphertext_hex, iv_hex, tag_hex, 16);

        std::cout << "Decrypted:    " << decrypted << "\n";

        // Verification
        if (decrypted == plaintext)
            std::cout << "[OK] Decryption matches original plaintext.\n";
        else
            std::cout << "[FAIL] Decrypted text does not match.\n";

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

int main(int argc, const char **argv)
{
    // TODO: add config
    logging::init(logging::LogVerbosity::highest);
    logging::info("Logger initialised");

    py::scoped_interpreter guard{};
    logging::info("Python scoped interpreter pybind11 initialised");

    /* For testing purposes only */
    if (argc > 1 && !strcmp(argv[1], "--module")) {
        logging::warn("[MODULE TESTER]: Running module tester. "
                "Module will be loaded and run. Then the program will exit");
        _module_testing(argc, argv);
        return 100;
    }

    /* For testing purposes only */
    if (argc > 1 && !strcmp(argv[1], "--security")) {
        logging::msg("Running application in simplified mode. Reason: Security handling");
        SecurityCLI cli;
        return cli.Enter(argc, argv);
    }

    Config &cfg = Config::getInstance("config.yaml");
    logging::msg("Configuration version: " + cfg.get<std::string>("version"));

    SecurityManager &sec = SecurityManager::getInstance();
    if (sec.VerifyPassword())
        std::cout << "OK";
    else  {
        std::cout << "Bad pass";
        return 1;
    }

    // TODO: startmonitoring will throw exceptions (check declaration), support that
    Monitor monitor;
    if (monitor.Initialise()) {
        monitor.StartMonitoring();
    }
    

    logging::msg("Exiting properly");
    return 0;
}

