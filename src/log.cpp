#include <Config.hpp>
#include <SecurityManager.hpp>
#include <CryptoUtil.hpp>
#include <Log.hpp>
#include <cstdint>
#include <ostream>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#else 
#include <pwd.h>
#include <unistd.h>
#endif

static std::string filepath = "";
static std::string logDir = "";
static logging::LogVerbosity verbosity = logging::LogVerbosity::normal;
static bool secureLogs = false;
static bool silentLogs = false;

namespace logging
{
    std::string LogDir() { return logDir; }

    bool init(logging::LogVerbosity v) {
        verbosity = v;
        secureLogs = false;
    
    #ifdef _WIN32
        char path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
            logDir = std::string(path) + "\\Monitor\\logs\\";
        } else {
            logDir = ".\\Monitor\\logs\\";
        }
    #else
        // --- Linux / Unix ---
        const char* home = std::getenv("HOME");
    
        // Handle sudo case: if running as root but SUDO_USER is set, use that user's home.
        const char* sudoUser = std::getenv("SUDO_USER");
        if (sudoUser && std::string(sudoUser).size() > 0 && geteuid() == 0) {
            struct passwd* pw = getpwnam(sudoUser);
            if (pw && pw->pw_dir) {
                home = pw->pw_dir;
            }
        }
    
        if (home && std::string(home).size() > 0) {
            logDir = std::string(home) + "/.local/state/monitor/logs/";
        } else {
            logDir = "./monitor/logs/";
        }
    #endif
    
        std::filesystem::create_directories(logDir);
    
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::ostringstream timestamp;
        timestamp << std::put_time(std::localtime(&now_time), "%Y-%m-%d_%H-%M-%S");
    
        filepath = logDir + timestamp.str() + "-Monitor.log";
    
        return true;
    }
    static std::string EncryptLine(const std::string &s)
    {
        SecurityManager &SecurityMgr = SecurityManager::getInstance();
        Config &Cfg = Config::getInstance();
    
        try {
            std::string csalt = SecurityMgr.GetCryptoSalt();
            uint32_t iterations = SecurityMgr.GetIterations();
        
            // std::string, std::string
            auto [ciphertext, tag] = AESUtil::AESGcmEncrypt(SecurityMgr.LogKey(), 32, s, SecurityMgr.GetLIV(), 12);
        
            return tag + ciphertext;
        } catch (const std::runtime_error &e) {
            std::cerr << "Failed to encrypt log: " << e.what() << std::endl;
            return "";
        }
    }
    
    static bool _log(const std::string& pre, const std::string& msg, bool printToCerr)
    {
        try {
            auto now = std::chrono::system_clock::now();
            std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    
            std::ostringstream timestamp;
            timestamp << std::put_time(std::localtime(&now_time), "%Y-%m-%d %H:%M:%S");
    
            std::string formatted = "[" + timestamp.str() + "] " + pre + ": " + msg;
    
            if (printToCerr && !silentLogs) {
                std::cerr << formatted << std::endl;
            }

            // If securelogs, we need to encrypt each line one by one, 
            // slow, but would require exceptionally shitty system to 
            // notice delay at which points its user error in my book
            if (secureLogs) {
                formatted = EncryptLine(formatted);
            }

            std::ofstream fout(filepath, std::ios::app);
            if (!fout.is_open()) {
                throw std::runtime_error("Cannot open log file: " + filepath);
            }
    
            fout << formatted << std::endl;

            if (!fout.good()) {
                throw std::runtime_error("Failed to write to log file: " + filepath);
            }

            fout.close();
        } 
        catch (const std::exception& e) {
            std::cerr << "Failed to log: " << e.what() << std::endl;
            return false;
        }
    
        return true;
    }
    
    bool err(std::string s)
    {
        return _log("ERROR", s, verbosity >= LogVerbosity::low);
    }
    
    bool warn(std::string s)
    {
        return _log("!WARN!", s, verbosity >= LogVerbosity::low);
    }
    
    bool msg(std::string s)
    {
        if (verbosity < LogVerbosity::normal)
            return false;
    
        return _log("Log", s, verbosity >= LogVerbosity::high);
    }
    
    bool info(std::string s)
    {
        if (verbosity < LogVerbosity::highest)
            return false;
    
        return _log("Info", s, verbosity >= LogVerbosity::highest);
    }
    
    bool setup()
    {
        Config &Cfg = Config::getInstance();

        // false by default
        secureLogs = Cfg.get<bool>("monitor.log.secure", false);
        // false by default
        silentLogs = Cfg.get<bool>("monitor.log.silent", false);
        // normal by default. 1 corresponds to logging::LogVerbosity::Normal
        uint32_t v = Cfg.get<uint32_t>("monitor.log.verbosity", 1);
        verbosity = static_cast<logging::LogVerbosity>(v);

        // TODO: Try to fix this issue where I must have encrypted config to have secure logs
        SecurityManager &SM = SecurityManager::getInstance();
        if (secureLogs && SM.LogKey() == "") {
            std::cerr << "Secure logs option (monitor.log.secure) cannot be used with unencrypted config. "
                "Please encrypt the configuration file with ./monitor --security encrypt config" << std::endl;
            return false;
        }

        return true;
    }
}
