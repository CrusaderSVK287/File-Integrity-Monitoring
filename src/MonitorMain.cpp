#include <Monitor.hpp>
#include <CryptoUtil.hpp>
#include <PortabilityUtils.hpp>
#include <Log.hpp>
#include <unistd.h>

extern volatile int _signal_Interrupt;

void Monitor::StartMonitoring()
{
    logging::msg("Starting logging session");

    while (!_signal_Interrupt) {
        logging::msg("Monitor running...");
        RunScan();
        SLEEP(m_u64period);
    }
}

int Monitor::RunScan()
{
    //- Compute a hash
    //- Retrieve a hash from database manager
    //    - If does not exist, store the hash in the database and continue to next file
    //- Verify that the hashes are valid
    //    - If not, send a notify alert to the mailing manager
    //- Continue with next file until all files done
    /*for (const std::string &file : m_files) {
           
    }*/

    std::string hash = ComputeHash("index.html");
    logging::msg(hash);

    return 0;
}

std::string Monitor::ComputeHash(const std::string &filename)
{
    return m_hashAlgorhitm(filename);
}
