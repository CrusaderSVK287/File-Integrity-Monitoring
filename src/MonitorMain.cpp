#include <Monitor.hpp>
#include <CryptoUtil.hpp>
#include <PortabilityUtils.hpp>
#include <Log.hpp>

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

    for (const std::string &file : m_files) {
        // Compute a hash
        std::string hashCompare = ComputeHash(file);
        logging::msg(hashCompare);

        // Retrieve a hash from database
        // TODO: finish when database plugin ready, store hash as basiline if doesnt exist
        py::dict result = QueryDatabase("SELECT hash FROM table WHERE key is whatever");
        std::string hashBaseline = result["hash"].cast<std::string>();

        if (hashCompare != hashBaseline) {
            //TODO: call email plugin to raise alert
            logging::warn("[Monitor] File " + file + " fingerprint does not match baseline, file may be compromised");
        }
    }

    return 0;
}

std::string Monitor::ComputeHash(const std::string &filename)
{
    return m_hashAlgorhitm(filename, m_filters);
}

// TODO: implement when database module ready
py::dict Monitor::QueryDatabase(const std::string &query)
{
    py::dict params;
    params["query"] = query;
    return Modules.RunModule("_dbtest", params);
}

