#include <Monitor.hpp>
#include <CryptoUtil.hpp>
#include <PortabilityUtils.hpp>
#include <Log.hpp>
#include <cstdint>
#include <DatabaseInterface.hpp>

using Q = DatabaseInterface::Action;

std::string hash8(const std::string& s) {
    std::size_t h = std::hash<std::string>{}(s);

    uint32_t short_hash = static_cast<uint32_t>(h);

    std::stringstream ss;
    ss << std::hex << std::setw(8) << std::setfill('0') << short_hash;
    return ss.str();
}

extern volatile int _signal_Interrupt;

void Monitor::StartMonitoring()
{
    logging::msg("Starting logging session");

    while (!_signal_Interrupt) {
        logging::msg("Running integrity scan");
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
        logging::info("Compare =  " + hashCompare);

        std::string filecode = hash8(file);

        // Retrieve a hash from database
        DatabaseQuery query = DatabaseInterface::Query(Modules, Q::SELECT, filecode);
        if (query["status"] != "OK") {
            logging::err("Database error: " + query["message"]);
            continue;    
        }
        
        std::string hashBaseline = query["hash"];
        logging::info("Baseline = " + hashBaseline);
        // This means that no baseline was found, so we insert the new baseline
        if (hashBaseline == "NULL") {
            logging::msg("Query for " + filecode + " returned NULL, inserting new baseline");
            query = DatabaseInterface::Query(Modules, Q::INSERT, filecode, hashCompare);
            if (query["status"] != "OK") {
                logging::err("Database error: " + query["message"]);
            }
            continue;
        }

        if (hashCompare != hashBaseline) {
            logging::warn("[Monitor] File " + file + " fingerprint does not match baseline, file may be compromised");

            if (m_MailingEnabled) {
                m_MailingManager->sendIncidentReport(filecode, "The computed fingerprint does not match an "
                        "entry in one or more database. \nFile on path '" + file + "' may be "
                        "compromised,\nit is recommended to verify the integrity of the files\n");
            }
        } else {
            // Handle resolved incidents
            if (m_MailingEnabled && m_MailingManager->isIncidentOngoing(filecode)) {
                if (m_MailingNotifyWhenResolved) {
                    m_MailingManager->sendIncidentResolved(filecode, "The incident has been resolved, further action may not be necessary\n");
                }
                m_MailingManager->markResolved(filecode);
            }
        }
    }

    return 0;
}

std::string Monitor::ComputeHash(const std::string &filename)
{
    return m_hashAlgorhitm->Run(filename, m_filters);
}

