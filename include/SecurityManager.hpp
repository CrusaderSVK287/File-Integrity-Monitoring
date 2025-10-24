#pragma once

#include <string>

class SecurityManager
{

public:
    SecurityManager(const SecurityManager&) = delete;
    SecurityManager& operator=(const SecurityManager&) = delete;

    static SecurityManager& getInstance();

    bool VerifyPassword();
    bool VerifyPassword(std::string password);
    
private:
    explicit SecurityManager();

    bool GenerateNewPasswordUserInput();

private:
    // TODO: portability and change to something normal
    const std::string m_sPwdFilePath = "/tmp/.password";
};
