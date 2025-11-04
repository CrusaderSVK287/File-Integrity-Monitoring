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
#include <yaml-cpp/node/parse.h>
#include <Config.hpp>

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

    try {
        InitialiseFilters();
    } catch (const std::runtime_error &e) {
        logging::err("Filter configuration is corrupted. Please review your configuration. The program will continue to run, but no filters will be used");
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

bool Monitor::InitialiseFilters()
{
    YAML::Node filterNode = Cfg.get<YAML::Node>("filter");

    for (const auto& entry : filterNode) {
        std::string filename = entry["file"].as<std::string>();
        std::string linesCSV = entry["lines"].as<std::string>();

        std::unordered_set<uint64_t> lines;
        std::stringstream ss(linesCSV);
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
                    lines.insert_range(std::views::iota(lower, upper + 1));
                } else {
                    // in case user is incapable of understanding child-like syntax
                    logging::err("Invalid format for filter range [" + token + "]. Use correct format [lower-upper]");
                    throw std::invalid_argument("");
                }
            }
            // handle single line filter
            else {
                uint64_t line = std::stoull(token);
                lines.insert(line);
            }
        }

        m_filters[filename] = std::move(lines);
    }

    return true;
}

bool Monitor::InitialiseConfig()
{
    // Period
    m_u64period = Cfg.get<uint64_t>("monitor.period");
    // Hashing algorhitm
    std::string hashAlgo = Cfg.get<std::string>("monitor.algorithm");
    m_hashAlgorhitm = &SHAFileUtil::SHA256;
    if (hashAlgo == "sha256")
        m_hashAlgorhitm = &SHAFileUtil::SHA256;
    else if (hashAlgo == "sha512")
        m_hashAlgorhitm = &SHAFileUtil::SHA512;
    else if (hashAlgo == "blake2s256")
        m_hashAlgorhitm = &SHAFileUtil::Blake2s256;
    else if (hashAlgo == "blake2s512")
        m_hashAlgorhitm = &SHAFileUtil::Blake2s512;
    else if (hashAlgo == "sha3_256")
        m_hashAlgorhitm = &SHAFileUtil::SHA3_256;
    else if (hashAlgo == "sha3_512")
        m_hashAlgorhitm = &SHAFileUtil::SHA3_512;
    else
        throw std::invalid_argument("Unsupported hash algorithm: " + hashAlgo);

    m_files = Cfg.get<std::vector<std::string>>("files");

    return true;
}
