#pragma once

#include "SecurityCLI.hpp"
#include <string>
#include <stdint.h>

class SecurityManager
{

friend SecurityCLI;

public:
    SecurityManager(const SecurityManager&) = delete;
    SecurityManager& operator=(const SecurityManager&) = delete;

    static SecurityManager& getInstance();
	static bool IsAdministrator();
    static std::string GetPassword();

    bool VerifyPassword();
    bool VerifyPassword(std::string password);

    std::string GetCryptoSalt();
    uint32_t GetIterations();
    std::string GetLogSalt();;
    std::string GetIV();
    std::string GetCIV();
    std::string GetLIV();
    std::string GetTag();
    bool SetTag(const std::string &tag);

    std::string LogKey();
    void SetLogKey(const std::string& k) {m_LogKey = k;}
    
private:
    explicit SecurityManager();

    bool GenerateNewPasswordUserInput();
    std::string GetPwdFilePath();
    std::string GetPwdFileConfigString(const std::string &s);

private:
    std::string m_LogKey = "";
};
