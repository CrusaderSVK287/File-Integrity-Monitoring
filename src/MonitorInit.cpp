#include <CryptoUtil.hpp>
#include <csignal>
#include <cstdint>
#include <Monitor.hpp>
#include <Log.hpp>
#include <memory>
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

    return true;
}
