#pragma once

#include <string>

class SecurityManager
{

public:
    SecurityManager(const SecurityManager&) = delete;
    SecurityManager& operator=(const SecurityManager&) = delete;

    static SecurityManager& getInstance();
	static bool IsAdministrator();

    bool VerifyPassword();
    bool VerifyPassword(std::string password);
    
private:
    explicit SecurityManager();

    bool GenerateNewPasswordUserInput();

private:
	// TODO: portability and change to something normal
	std::string m_sPwdFilePath = "%LOCALAPPDATA%\\Monitor\\pwd.yaml";
};
