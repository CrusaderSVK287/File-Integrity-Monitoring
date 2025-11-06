#pragma once

#include <string>
#include <stdint.h>

class SecurityManager
{

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
    std::string GetIV();
    std::string GetTag();
    bool SetTag(const std::string &tag);
    
private:
    explicit SecurityManager();

    bool GenerateNewPasswordUserInput();
};
