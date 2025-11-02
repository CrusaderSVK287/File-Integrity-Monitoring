#include <Log.hpp>
#include <ostream>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#endif

static std::string filepath = "";
static logging::LogVerbosity verbosity = logging::LogVerbosity::normal;

namespace logging
{
    bool init(LogVerbosity v)
    {
        verbosity = v;
#ifdef _WIN32
		char path[MAX_PATH];
		if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
			std::string dir = std::string(path) + "\\Monitor";
			CreateDirectoryA(dir.c_str(), NULL);
			filepath = dir + "\\monitor.log";
		}
#else
        filepath = "/tmp/monitor.log";
#endif

        return true;
    }
    
    static bool _log(const std::string& pre, const std::string& msg, bool printToCerr)
    {
        try {
            auto now = std::chrono::system_clock::now();
            std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    
            std::ostringstream timestamp;
            timestamp << std::put_time(std::localtime(&now_time), "%Y-%m-%d %H:%M:%S");
    
            std::string formatted = "[" + timestamp.str() + "] " + pre + ": " + msg;
    
            std::ofstream fout(filepath, std::ios::app);
            if (!fout.is_open()) {
                throw std::runtime_error("Cannot open log file: " + filepath);
            }
    
            fout << formatted << std::endl;
    
            if (!fout.good()) {
                throw std::runtime_error("Failed to write to log file: " + filepath);
            }
    
            if (printToCerr) {
                std::cerr << formatted << std::endl;
            }
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
    
        return _log("Log", s, verbosity >= LogVerbosity::highest);
    }
    
}
