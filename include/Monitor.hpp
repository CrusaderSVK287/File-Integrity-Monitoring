#pragma once

#include <MailAlertManager.hpp>
#include <HashingAlgorithm.hpp>
#include <SecurityManager.hpp>
#include <pybind11/embed.h>
#include <ModuleManager.hpp>
#include <Config.hpp>
#include <Filters.hpp>
#include <cstdint>
#include <vector>

class Monitor {
    public:
        Monitor() :
            Security(SecurityManager::getInstance()),
            Modules(ModuleManager()),
            Cfg(Config::getInstance()),
            m_hashAlgorhitm(nullptr),
            m_MailingManager(nullptr),
            m_MailingEnabled(false)
        {}

        bool Initialise();

        // Main function of the program, will throw exception if anything weird happens.
        // Should be restarted right away. Keep track of how many restarts and how often.
        // If too many in too short of a time, raise an error, send email and stop program.
        void StartMonitoring();

    private:
        bool InitialiseModules(); 
        bool InitialiseConfig(); // for stuff like time period between checks etc
        bool InitialiseFilters();
        bool InitialiseMailing();
        std::string ComputeHash(const std::string &s);    // Algorhitm agnostic method that calls m_hashAlgorhitm with algorhitm set up in config

        int RunScan();

        py::dict QueryDatabase(const std::string &query);

    private:
        // Managers
        ModuleManager Modules;
        SecurityManager &Security;
        Config &Cfg;

    private:
        // Configs
        uint64_t m_u64period = 0;               // Time period between each scans
        std::vector<std::string> m_files;       // Filenames to be monitored
        HashingAlgorithm *m_hashAlgorhitm;           // Pointer to the function used for checksumming the files
        FilterMap m_filters;
        bool m_MailingEnabled;
        MailAlertManager *m_MailingManager;
};
