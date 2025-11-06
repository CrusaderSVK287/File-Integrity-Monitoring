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

std::string getPwdFilePath() {
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
void SetStdinEcho(bool enable) {
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    if (!enable)
        tty.c_lflag &= ~ECHO;
    else
        tty.c_lflag |= ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

std::string getPwdFilePath() {
	return "pwd.yaml";
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
    std::cout << "Enter password: ";
    SetStdinEcho(false);
    std::string input;
    std::getline(std::cin, input);
    SetStdinEcho(true);
    return input;
}

bool SecurityManager::VerifyPassword()
{
    return VerifyPassword(SecurityManager::GetPassword());
}

bool SecurityManager::VerifyPassword(std::string password)
{
    try {
        YAML::Node yaml = YAML::LoadFile(getPwdFilePath());
        std::string s_SaltHex = yaml["users"]["admin"]["salt"].as<std::string>();
        std::string s_DkHex = yaml["users"]["admin"]["dk"].as<std::string>();
        int iterations = yaml["users"]["admin"]["iterations"].as<int>();

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
    // derive key
    std::vector<unsigned char> dk = PBKDF2Util::DeriveKey(password, salt, iterations, 32);

    YAML::Node yaml;
    yaml["users"]["admin"]["salt"] = PBKDF2Util::ToHex(salt.data(), salt.size());
    yaml["users"]["admin"]["csalt"] = PBKDF2Util::ToHex(csalt.data(), csalt.size());
    yaml["users"]["admin"]["iv"] = AESUtil::GenerateIV(12);
    yaml["users"]["admin"]["dk"] = PBKDF2Util::ToHex(dk.data(), dk.size());
    yaml["users"]["admin"]["iterations"] = iterations;

    std::ofstream fout(getPwdFilePath());
    fout << yaml;

    logging::warn("Password has been changed, please verify that this action is ok");

    return true;
}

std::string SecurityManager::GetCryptoSalt()
{
    try {
        YAML::Node yaml = YAML::LoadFile(getPwdFilePath());
        return yaml["users"]["admin"]["csalt"].as<std::string>();
    } catch (...) {
        return "";
    }
}

uint32_t SecurityManager::GetIterations()
{
    try {
        YAML::Node yaml = YAML::LoadFile(getPwdFilePath());
        return yaml["users"]["admin"]["iterations"].as<uint32_t>();
    } catch (...) {
        return 0;
    }
}

std::string SecurityManager::GetIV()
{
    try {
        YAML::Node yaml = YAML::LoadFile(getPwdFilePath());
        return yaml["users"]["admin"]["iv"].as<std::string>();
    } catch (...) {
        return "";
    }
}

std::string SecurityManager::GetTag()
{
    try {
        YAML::Node yaml = YAML::LoadFile(getPwdFilePath());
        return yaml["users"]["admin"]["tag"].as<std::string>();
    } catch (...) {
        return "";
    }
}

bool SecurityManager::SetTag(const std::string &tag)
{
    try {
        YAML::Node yaml = YAML::LoadFile(getPwdFilePath());
        yaml["users"]["admin"]["tag"] = tag;

        std::ofstream fout(getPwdFilePath(), std::ios::out | std::ios::trunc);
        fout << yaml;

        return true;
    } catch (const std::exception &e) {
        logging::err(e.what());
        return false;
    }
}
