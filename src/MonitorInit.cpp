#include "MailAlertManager.hpp"
#include <HashingAlgorithm.hpp>
#include <CryptoUtil.hpp>
#include <csignal>
#include <cstdint>
#include <Monitor.hpp>
#include <Log.hpp>
#include <memory>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <yaml-cpp/exceptions.h>
#include <yaml-cpp/node/parse.h>
#include <Config.hpp>
#include <vector>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#endif

// non-static since used elsewhere
volatile int _signal_Interrupt = 0;

void sighandler_Interrupt(int signum)
{
    (void)signum;
    _signal_Interrupt = 1;
}

bool Monitor::Initialise()
{
    _signal_Interrupt = 0;
    if (signal(SIGINT, sighandler_Interrupt) == SIG_ERR) {
        logging::err("Failed to set up signal handlers");
        return false;
    }

    bool result = InitialiseModules();
    if (!result) {
        logging::err("Failed to initialise one or more modules");
        return false;
    }

    result = InitialiseConfig();
    if (!result) {
        logging::err("Failed to initialise one or more config values");
        return false;
    }
    
    // Mailing should not throw any exceptions because its not a mandatory module
    result = InitialiseMailing();
    if (!result) {
        logging::err("Failed to initialise Mailing Manager, review your configuration");
        return false;
    }

    try {
        InitialiseFilters();
    } catch (const std::runtime_error &e) {
        logging::warn("Filter configuration is corrupted. Please review your configuration. The program will continue to run, but no filters will be used");
        m_filters.clear();
    }

    return true;
}

bool Monitor::InitialiseModules()
{
    py::dict _emptyParams;
    bool result = Modules.LoadModule("_dbtest", _emptyParams);

    //TODO: normalny mailing module pridat. 
    //result = result & Modules.LoadModule("_mailtest", _emptyParams);

    return result;
}

static void FilterLinesPopulateSet(std::unordered_set<uint64_t> &set, const std::string &csv)
{
        std::stringstream ss(csv);
        std::string token;

        while (std::getline(ss, token, ',')) {
            // handle line range X-Y
            if (token.contains('-'))
            {
                uint64_t lower = 0, upper = 0;
                char dash;

                std::istringstream iss(token);
                if (iss >> lower >> dash >> upper && dash == '-' && lower < upper) {
                    // range string is valid
                    set.insert_range(std::views::iota(lower, upper + 1));
                } else {
                    // in case user is incapable of understanding child-like syntax
                    throw std::invalid_argument("Invalid format for filter range [" + token + "]. Use correct format [lower-upper]");
                }
            }
            // handle single line filter
            else {
                uint64_t line = std::stoull(token);
                set.insert(line);
            }
        }
}

bool Monitor::InitialiseFilters()
{
    YAML::Node filterNode;
    try {
        filterNode = Cfg.get<YAML::Node>("filter");
    } catch (const std::runtime_error &e) {
        // No filters are going to be applied because none exist
        return false;
    }
    if (filterNode.IsNull())
        // No filters are going to be applied because none exist
        return false;

    for (const auto& entry : filterNode) {
        std::string type = entry["type"].as<std::string>();

        if (type == "lines") {
            std::string filename = entry["file"].as<std::string>();
            std::string linesCSV = entry["lines"].as<std::string>();
    
            logging::info("Setting up Lines filter for " + filename);
            std::unordered_set<uint64_t> lines;
            FilterLinesPopulateSet(lines, linesCSV);
            
            m_filters[filename].push_back(std::make_unique<FilterLines>(filename, lines));
        } else if (type == "segment") {
            std::string filename    = entry["file"].as<std::string>();
            std::string start       = entry["start"].as<std::string>();
            std::string end         = entry["end"].as<std::string>();
            uint64_t line           = entry["line"].as<uint64_t>();
            bool all = false; // default is false
            if (entry["all"]) 
                all = entry["all"].as<bool>();
            
            m_filters[filename].push_back(std::make_unique<FilterSegment>(filename, start, end, all, line));
        } else {
            throw std::invalid_argument("Unknown filter type: " + type);
        }
    }

    return true;
}

bool Monitor::InitialiseConfig()
{
    // Period
    m_u64period = Cfg.get<uint64_t>("monitor.period", 60);
    // Hashing algorhitm
    std::string hashAlgo = Cfg.get<std::string>("monitor.algorithm", "sha");
    uint32_t key_lenght = Cfg.get<uint32_t>("monitor.key_length", 256);

    if (key_lenght != 256 && key_lenght != 512)
        throw std::invalid_argument("Invalid key lenght. Only 256 and 512 is supported");

    if (hashAlgo == "sha" && key_lenght == 256)
        m_hashAlgorhitm = new HashingAlgorithmSHA256();
    else if (hashAlgo == "sha" && key_lenght == 512)
        m_hashAlgorhitm = new HashingAlgorithmSHA512();
    else if (hashAlgo == "blake2s" && key_lenght == 256)
        m_hashAlgorhitm = new HashingAlgorithmBlake2s256();
    else if (hashAlgo == "blake2s" && key_lenght == 512)
        m_hashAlgorhitm = new HashingAlgorithmBlake2s512();
    else if (hashAlgo == "sha3" && key_lenght == 256)
        m_hashAlgorhitm = new HashingAlgorithmSHA3_256();
    else if (hashAlgo == "sha3" && key_lenght == 512)
        m_hashAlgorhitm = new HashingAlgorithmSHA3_512();
    else
        throw std::invalid_argument("Unsupported hash algorithm: " + hashAlgo);

    try {
        m_files = Cfg.get<std::vector<std::string>>("files");
    } catch (const YAML::BadConversion &e) {
        // When we get BadConversion, it most likely means that
        // no files were provided. YAML expects to return a 
        // vector of strings but there is no value at all
        throw std::invalid_argument("You need to provide at least one file for monitoring");
    }
    if (m_files.size() == 0) {
        throw std::invalid_argument("You need to provide at least one file for monitoring");
    }

    return true;
}

bool Monitor::InitialiseMailing()
{
    // Enable. By default false
    m_MailingEnabled = Cfg.get<bool>("mailing.enable", false);
    if (!m_MailingEnabled) {
        // even tho m_MailingEnabled is false, we return true since this is expected behaviour
        return true;
    }

    // mandatory params
    std::string mailUser = Cfg.get<std::string>("mailing.user", "");
    std::string mailPassword = Cfg.get<std::string>("mailing.password", "");
    std::vector<std::string> mailList = Cfg.get<std::vector<std::string>>("mailing.list");
    if (mailUser == "" || mailPassword == "" || mailList.empty()) {
        return false;
    }

    // Optional params
    uint32_t maxEmails = Cfg.get<uint32_t>("mailing.limit", 3); // emails per incident
    uint32_t minInterval = Cfg.get<uint32_t>("mailing.spacing", 600); // minimal interval between emails
    uint32_t resetInterval = Cfg.get<uint32_t>("mailing.reset", 3600); // reset number of emails sent for a particular 
                                                                       // incident after a certain period of time
    m_MailingManager = new MailAlertManager(
        mailList,
        maxEmails,
        MailAlertManager::Duration(resetInterval),
        MailAlertManager::Duration(minInterval),
        false, // dry run, we want to actually send emails
        mailUser,
        mailPassword
    );

    return true;
}

