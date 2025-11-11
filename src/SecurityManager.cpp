#include <SecurityManager.hpp>
#include <CryptoUtil.hpp>
#include <Log.hpp>

#include <cctype>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/node/parse.h>
#include <yaml-cpp/yaml.h>
#include <iostream>
#include <filesystem>
#include <string>

#ifdef _WIN32
#define _WIN32_WINNT 0x0501
#include <windows.h>
#include <shlobj.h>

void  SetStdinEcho(bool enable) {
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE); 
    DWORD mode;
    GetConsoleMode(hStdin, &mode);
    if (!enable) mode &= ~ENABLE_ECHO_INPUT;
    else mode |= ENABLE_ECHO_INPUT;
    SetConsoleMode(hStdin, mode);
}

std::string SecurityManager::GetPwdFilePath() {
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
        std::string dir = std::string(path) + "\\Monitor";
        CreateDirectoryA(dir.c_str(), NULL);
        return dir + "\\pwd.yaml";
    }
    return "pwd.yaml";
}

#else /* ifdef _WIN32 */
#include <termios.h>
#include <pwd.h>
#include <unistd.h>
void SetStdinEcho(bool enable) {
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    if (!enable)
        tty.c_lflag &= ~ECHO;
    else
        tty.c_lflag |= ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

std::string SecurityManager::GetPwdFilePath() {
    const std::string pwdFileName = "pwd.yaml";
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
        std::string idk = home;
        idk = idk + "/.local/state/monitor";
        std::filesystem::create_directories(idk);
        return std::string(home) + "/.local/state/monitor/" + pwdFileName;
    } else {
	    return pwdFileName;
    }
}
#endif /* ifdef _WIN32 */

// static
bool SecurityManager::IsAdministrator()
{
#ifdef WIN32
    // code from https://vimalshekar.github.io/codesamples/Checking-If-Admin

    BOOL fIsElevated = FALSE;
    HANDLE hToken = NULL;
    TOKEN_ELEVATION elevation;
    DWORD dwSize;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        goto Cleanup;  // if Failed, we treat as False
    }


    if (!GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &dwSize))
    {        goto Cleanup;// if Failed, we treat as False
    }

    fIsElevated = elevation.TokenIsElevated;

Cleanup:
    if (hToken)
    {
        CloseHandle(hToken);
        hToken = NULL;
    }
    return fIsElevated; 
#else
    return geteuid() == 0;
#endif

}

SecurityManager& SecurityManager::getInstance() {
    static SecurityManager instance;
    return instance;
}

SecurityManager::SecurityManager() {
}

std::string SecurityManager::GetPassword()
{
    std::cout << "Enter password: " << std::flush;

    SetStdinEcho(false);
    std::cin >> std::ws;

    std::string input;
    std::getline(std::cin, input);

    SetStdinEcho(true);
    std::cout << std::endl;

    // Trim a trailing CR if present (Windows CRLF -> getline may leave '\r')
    if (!input.empty() && input.back() == '\r') {
        input.pop_back();
    }

    return input;
}

bool SecurityManager::VerifyPassword()
{
    return VerifyPassword(GetPassword());
}

bool SecurityManager::VerifyPassword(std::string password)
{
    try {
        YAML::Node yaml = YAML::LoadFile(GetPwdFilePath());
        std::string s_SaltHex = yaml["salt"].as<std::string>();
        std::string s_DkHex = yaml["dk"].as<std::string>();
        int iterations = yaml["iterations"].as<int>();

        std::vector<unsigned char> salt = PBKDF2Util::FromHex(s_SaltHex);
        std::vector<unsigned char> dk = PBKDF2Util::FromHex(s_DkHex);

        return PBKDF2Util::VerifyPassword(password, salt, dk, iterations);
    }catch (const YAML::BadFile &e) {
        logging::err("YAML: The file containing the passowrd does not exist");
        return GenerateNewPasswordUserInput();
    }
    catch (const YAML::ParserException &e) {
        logging::err("YAML: The file containing the password is corrupted");
        return GenerateNewPasswordUserInput();
    }
    catch (const std::exception &e) {
        logging::err(e.what());
        return false;
    }
    // just in case
    return false;
}

bool SecurityManager::GenerateNewPasswordUserInput()
{
    std::cout << "Would you like to generate new password? [y/N]: ";
    std::string input;
    std::getline(std::cin, input);
    std::cout << std::endl;
    if (input.length() == 0 || std::toupper(input[0]) != 'Y') {
        return false;
    }

	if (!SecurityManager::IsAdministrator()) {
        std::cerr << "You need to have administrator/root priviledges to be able to set new password" << std::endl;
        return false;
    }

    logging::warn("Password Change in progress");
	
    std::cout << "Enter new password: ";
    SetStdinEcho(false);
    std::getline(std::cin, input);
    std::cout << std::endl << "Confirm password:";
    std::string password;
    std::getline(std::cin, password);
    std::cout << std::endl;
    SetStdinEcho(true);

    if (input != password) {
        std::cout << "Passwords do not match!" << std::endl;
        return false;
    }

    uint32_t iterations = 100000;
    // Salt
    std::vector<unsigned char> salt = PBKDF2Util::GenerateSalt(16);
    std::vector<unsigned char> csalt = PBKDF2Util::GenerateSalt(16);
    std::vector<unsigned char> lsalt = PBKDF2Util::GenerateSalt(16);
    // derive key
    std::vector<unsigned char> dk = PBKDF2Util::DeriveKey(password, salt, iterations, 32);

    YAML::Node yaml;
    yaml["salt"] = PBKDF2Util::ToHex(salt.data(), salt.size());
    yaml["csalt"] = PBKDF2Util::ToHex(csalt.data(), csalt.size());
    yaml["lsalt"] = PBKDF2Util::ToHex(lsalt.data(), lsalt.size());
    yaml["iv"] = AESUtil::GenerateIV(12);
    yaml["civ"] = AESUtil::GenerateIV(12);
    yaml["liv"] = AESUtil::GenerateIV(12);
    yaml["dk"] = PBKDF2Util::ToHex(dk.data(), dk.size());
    yaml["iterations"] = iterations;

    std::ofstream fout(GetPwdFilePath());
    fout << yaml;

    logging::warn("Password has been changed, please verify that this action is ok");

    return true;
}

std::string SecurityManager::GetPwdFileConfigString(const std::string &s)
{
    try {
        YAML::Node yaml = YAML::LoadFile(GetPwdFilePath());
        return yaml[s].as<std::string>();
    } catch (...) {
        return "";
    }
}

std::string SecurityManager::GetCryptoSalt(){return GetPwdFileConfigString("csalt");}
std::string SecurityManager::GetLogSalt(){return GetPwdFileConfigString("lsalt");}
std::string SecurityManager::GetIV(){return GetPwdFileConfigString("iv");}
std::string SecurityManager::GetCIV(){return GetPwdFileConfigString("civ");}
std::string SecurityManager::GetLIV(){return GetPwdFileConfigString("liv");}
std::string SecurityManager::GetTag() {return GetPwdFileConfigString("tag");}

uint32_t SecurityManager::GetIterations()
{
    try {
        YAML::Node yaml = YAML::LoadFile(GetPwdFilePath());
        return yaml["iterations"].as<uint32_t>();
    } catch (...) {
        return 0;
    }
}

bool SecurityManager::SetTag(const std::string &tag)
{
    try {
        YAML::Node yaml = YAML::LoadFile(GetPwdFilePath());
        yaml["tag"] = tag;

        std::ofstream fout(GetPwdFilePath(), std::ios::out | std::ios::trunc);
        fout << yaml;

        return true;
    } catch (const std::exception &e) {
        logging::err(e.what());
        return false;
    }
}

std::string SecurityManager::LogKey()
{
    return m_LogKey;
}

