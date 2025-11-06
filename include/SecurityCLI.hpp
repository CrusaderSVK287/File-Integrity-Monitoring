#pragma once

class SecurityCLI {
    public:

        // When --security parameter is provided, call this and then exit
        int Enter(const int argc, const char **argv);

    private:

        int EncryptConfig();
        int DecryptConfig();

        int DecryptLogs();

        int Verify();
        int Usage();
};
