#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <optional>
#include <mutex>

class MailAlertManager {
public:
    using Clock = std::chrono::system_clock;
    using Duration = std::chrono::seconds;
    using TimePoint = std::chrono::time_point<Clock>;

    struct IncidentState {
        int count = 0;
        std::optional<TimePoint> lastSent;
    };

    MailAlertManager(
        std::vector<std::string> mailingList,
        int maxEmailsPerIncident,
        Duration resetAfter,
        Duration minIntervalBetweenEmails,
        bool dryRun,
        std::string gmailUser,         // napr. "tvojucet@gmail.com"
        std::string gmailAppPassword   // 16-znakový App Password
    );

    bool sendIncidentReport(const std::string& incidentId, const std::string& msg);
    bool sendIncidentResolved(const std::string& incidentId, const std::string& msg);

    void markResolved(const std::string& incidentId);

    void resetAll();

    bool isIncidentOngoing(const std::string &incidentId);

private:
    std::vector<std::string> mailingList_;
    int maxEmailsPerIncident_;
    Duration resetAfter_;
    Duration minIntervalBetweenEmails_;
    bool dryRun_;

    std::string gmailUser_;
    std::string gmailAppPassword_;

    std::unordered_map<std::string, IncidentState> incidents_;
    std::mutex mutex_;

    void sendEmail(const std::string& subject, const std::string& body);
};
