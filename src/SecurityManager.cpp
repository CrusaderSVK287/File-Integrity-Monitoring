#include <SecurityManager.hpp>
#include <CryptoUtil.hpp>

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
#include <windows.h>
void  SetStdinEcho(bool enable) {
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE); 
    DWORD mode;
    GetConsoleMode(hStdin, &mode);
    if (!enable) mode &= ~ENABLE_ECHO_INPUT;
    else mode |= ENABLE_ECHO_INPUT;
    SetConsoleMode(hStdin, mode);
}
#else
#include <termios.h>
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
#endif

SecurityManager& SecurityManager::getInstance() {
    static SecurityManager instance;
    return instance;
}

SecurityManager::SecurityManager() {
}

bool SecurityManager::VerifyPassword()
{
    std::cout << "Enter password: ";
    SetStdinEcho(false);
    std::string input;
    std::getline(std::cin, input);
    SetStdinEcho(true);
    return VerifyPassword(input);
}

bool SecurityManager::VerifyPassword(std::string password)
{
    try {
        YAML::Node yaml = YAML::LoadFile(m_sPwdFilePath);
        std::string s_SaltHex = yaml["users"]["admin"]["salt"].as<std::string>();
        std::string s_DkHex = yaml["users"]["admin"]["dk"].as<std::string>();
        int iterations = yaml["users"]["admin"]["iterations"].as<int>();

        std::vector<unsigned char> salt = PBKDF2Util::FromHex(s_SaltHex);
        std::vector<unsigned char> dk = PBKDF2Util::FromHex(s_DkHex);

        return PBKDF2Util::VerifyPassword(password, salt, dk, iterations);
    }catch (const YAML::BadFile &e) {
        std::cerr << "The file containing the passowrd does not exist" << std::endl;
        return GenerateNewPasswordUserInput();
    }
    catch (const YAML::ParserException &e) {
        std::cerr << "The file containing the password is corrupted" << std::endl;
        return GenerateNewPasswordUserInput();
    }
    catch (const std::exception &e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
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
    // derive key
    std::vector<unsigned char> dk = PBKDF2Util::DeriveKey(password, salt, iterations, 32);

    YAML::Node yaml;
    yaml["users"]["admin"]["salt"] = PBKDF2Util::ToHex(salt.data(), salt.size());
    yaml["users"]["admin"]["dk"] = PBKDF2Util::ToHex(dk.data(), dk.size());
    yaml["users"]["admin"]["iterations"] = iterations;
    
    std::ofstream fout(m_sPwdFilePath);
    fout << yaml;

    return true;
}
