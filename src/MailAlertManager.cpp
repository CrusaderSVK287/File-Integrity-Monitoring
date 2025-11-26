#include <MailAlertManager.hpp>
#include <Log.hpp>

#include <iostream>
#include <sstream>
#include <cstdio>
#include <ctime>
#include <cstring>

#ifdef _WIN32
  #define CLOSE_SOCKET_AND_CLEANUP(s) do { closesocket(s); WSACleanup(); } while(0)
#else
  #define CLOSE_SOCKET_AND_CLEANUP(s) do { close(s); } while(0)
#endif

#ifdef _WIN32
    #define _WINSOCK_DEPRECATED_NO_WARNINGS
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
#endif

// OpenSSL
#include <openssl/ssl.h>
#include <openssl/err.h>

// ------------------ Helper: Base64 (potrebné pre AUTH LOGIN) ------------------

static const std::string BASE64_CHARS =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

static std::string base64Encode(const std::string& in) {
    std::string out;
    int val = 0;
    int valb = -6;
    for (unsigned char c : in) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(BASE64_CHARS[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(BASE64_CHARS[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

// ------------------ Helper: SMTP cez SSL ------------------

static bool sslWriteLine(SSL* ssl, const std::string& line) {
    std::string data = line + "\r\n";
    int ret = SSL_write(ssl, data.c_str(), static_cast<int>(data.size()));
    return (ret > 0);
}

static bool sslReadResponse(SSL* ssl) {
    char buf[4096];
    int ret = SSL_read(ssl, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        return false;
    }
    buf[ret] = '\0';
    logging::info("[SMTP] " + std::string(buf));  // logni si odpoveď
    return true;
}

// Jednoduché odoslanie cez Gmail SMTP (smtp.gmail.com:465, TLS od začiatku)
static bool sendViaGmailSmtp(
    const std::string& gmailUser,
    const std::string& gmailAppPassword,
    const std::string& fromAddress,
    const std::vector<std::string>& recipients,
    const std::string& rawMessage
) {
    const char* smtpHost = "smtp.gmail.com";
    const int   smtpPort = 465;

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        logging::err("[MailAlertManager] WSAStartup failed.\n");
        return false;
    }

    addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* result = nullptr;
    if (getaddrinfo(smtpHost, "465", &hints, &result) != 0) {
        logging::err("[MailAlertManager] getaddrinfo failed.\n");
        WSACleanup();
        return false;
    }

    SOCKET sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sock == INVALID_SOCKET) {
        logging::err("[MailAlertManager] socket() failed.\n");
        freeaddrinfo(result);
        WSACleanup();
        return false;
    }

    if (connect(sock, result->ai_addr, static_cast<int>(result->ai_addrlen)) == SOCKET_ERROR) {
        logging::err("[MailAlertManager] connect() failed.\n");
        closesocket(sock);
        freeaddrinfo(result);
        WSACleanup();
        return false;
    }

    freeaddrinfo(result);

#else
    struct hostent* he = gethostbyname(smtpHost);
    if (!he) {
        logging::err("[MailAlertManager] gethostbyname failed.\n");
        return false;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        logging::err("[MailAlertManager] socket() failed.\n");
        return false;
    }

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(smtpPort);
    std::memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        logging::err("[MailAlertManager] connect() failed.\n");
        close(sock);
        return false;
    }
#endif


    // 3) OpenSSL init
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
    const SSL_METHOD* method = TLS_client_method();
    SSL_CTX* ctx = SSL_CTX_new(method);
	if (!ctx) {
		logging::err("[MailAlertManager] SSL_CTX_new failed.\n");
		CLOSE_SOCKET_AND_CLEANUP(sock);
		return false;
	}


    SSL* ssl = SSL_new(ctx);
	SSL_set_fd(ssl, static_cast<int>(sock));
	if (SSL_connect(ssl) <= 0) {
		logging::err("[MailAlertManager] SSL_connect failed.\n");
		SSL_free(ssl);
		SSL_CTX_free(ctx);
		CLOSE_SOCKET_AND_CLEANUP(sock);
		return false;
	}


    // 4) SMTP handshake
    if (!sslReadResponse(ssl)) { // server greeting
        logging::err("[MailAlertManager] No greeting.\n");
    }

    if (!sslWriteLine(ssl, "EHLO localhost") || !sslReadResponse(ssl)) {
        logging::err("[MailAlertManager] EHLO failed.\n");
    }

    // 5) AUTH LOGIN
    if (!sslWriteLine(ssl, "AUTH LOGIN") || !sslReadResponse(ssl)) {
        logging::err("[MailAlertManager] AUTH LOGIN start failed.\n");
    }

    std::string userB64 = base64Encode(gmailUser);
    if (!sslWriteLine(ssl, userB64) || !sslReadResponse(ssl)) {
        logging::err("[MailAlertManager] AUTH LOGIN user failed.\n");
    }

    std::string passB64 = base64Encode(gmailAppPassword);
    if (!sslWriteLine(ssl, passB64) || !sslReadResponse(ssl)) {
        logging::err("[MailAlertManager] AUTH LOGIN password failed.\n");
    }

    // 6) MAIL FROM
    std::string mailFromCmd = "MAIL FROM:<" + fromAddress + ">";
    if (!sslWriteLine(ssl, mailFromCmd) || !sslReadResponse(ssl)) {
        logging::err("[MailAlertManager] MAIL FROM failed.\n");
    }

    // 7) RCPT TO pre každého príjemcu
    for (const auto& rcpt : recipients) {
        std::string rcptCmd = "RCPT TO:<" + rcpt + ">";
        if (!sslWriteLine(ssl, rcptCmd) || !sslReadResponse(ssl)) {
            logging::err("[MailAlertManager] RCPT TO failed for " + rcpt + "\n");
        }
    }

    // 8) DATA
    if (!sslWriteLine(ssl, "DATA") || !sslReadResponse(ssl)) {
        logging::err("[MailAlertManager] DATA failed.\n");
    }

    // 9) Poslať rawMessage + "\r\n.\r\n"
    std::string dataToSend = rawMessage + "\r\n.\r\n";
    if (SSL_write(ssl, dataToSend.c_str(), static_cast<int>(dataToSend.size())) <= 0) {
        logging::err("[MailAlertManager] SSL_write DATA failed.\n");
    }
    sslReadResponse(ssl); // response na DATA

    // 10) QUIT
    sslWriteLine(ssl, "QUIT");
    sslReadResponse(ssl);

    // 11) Cleanup
    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
#ifdef _WIN32
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif

	EVP_cleanup();


    return true;
}

// ------------------ MailAlertManager implementácia ------------------

MailAlertManager::MailAlertManager(
    std::vector<std::string> mailingList,
    int maxEmailsPerIncident,
    Duration resetAfter,
    Duration minIntervalBetweenEmails,
    bool dryRun,
    std::string gmailUser,
    std::string gmailAppPassword
)
    : mailingList_(std::move(mailingList)),
      maxEmailsPerIncident_(maxEmailsPerIncident),
      resetAfter_(resetAfter),
      minIntervalBetweenEmails_(minIntervalBetweenEmails),
      dryRun_(dryRun),
      gmailUser_(std::move(gmailUser)),
      gmailAppPassword_(std::move(gmailAppPassword))
{
}

bool MailAlertManager::sendIncidentResolved(const std::string& incidentId, const std::string& msg)
{
    std::lock_guard<std::mutex> lock(mutex_);

    const auto now = Clock::now();

    std::ostringstream subject;
    subject << "[INFORMATION] Incident " << incidentId << " resolved";

    std::ostringstream body;
    auto now_time_t = Clock::to_time_t(now);

    body << "Incident ID: " << incidentId << "\n";
    body << "Time: " << std::ctime(&now_time_t);
    body << "\nMessage:\n";
    body << msg << "\n\n";

    sendEmail(subject.str(), body.str());

    return true;
}

bool MailAlertManager::sendIncidentReport(const std::string& incidentId, const std::string& msg)
{
    std::lock_guard<std::mutex> lock(mutex_);

    const auto now = Clock::now();

    auto& state = incidents_[incidentId];

    // 1) Reset counter po dlhšej dobe neaktivity
    if (state.lastSent.has_value()) {
        auto diff = now - state.lastSent.value();
        if (diff >= resetAfter_) {
            state.count = 0;
            state.lastSent.reset();
        }
    }

    // 2) Limit počtu mailov
    if (state.count >= maxEmailsPerIncident_) {
        return false;
    }

    // 3) Minimálny interval medzi mailami
    if (state.lastSent.has_value()) {
        auto sinceLast = now - state.lastSent.value();
        if (sinceLast < minIntervalBetweenEmails_) {
            return false;
        }
    }

    int currentCount = state.count + 1;
    bool isLast = (currentCount == maxEmailsPerIncident_);

    std::ostringstream subject;
    subject << "[ALERT] Incident " << incidentId
            << " (" << currentCount << "/" << maxEmailsPerIncident_ << ")";

    std::ostringstream body;
    auto now_time_t = Clock::to_time_t(now);

    body << "Incident ID: " << incidentId << "\n";
    body << "Time: " << std::ctime(&now_time_t);
    body << "\nMessage:\n";
    body << msg << "\n\n";
    body << "This is alert #" << currentCount
         << " of " << maxEmailsPerIncident_
         << " for this incident.\n";

    if (isLast) {
        body << "\nNOTE: Maximum number of alerts for this incident has been reached.\n"
             << "No further emails will be sent for this incident until the counter is\n"
             << "reset (no alerts for "
             << std::chrono::duration_cast<std::chrono::minutes>(resetAfter_).count()
             << " minutes) or a manual reset.\n";
    }

    sendEmail(subject.str(), body.str());

    state.count = currentCount;
    state.lastSent = now;

    return true;
}

void MailAlertManager::markResolved(const std::string& incidentId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    incidents_.erase(incidentId);
}

void MailAlertManager::resetAll()
{
    std::lock_guard<std::mutex> lock(mutex_);
    incidents_.clear();
}

bool MailAlertManager::isIncidentOngoing(const std::string &incidentId)
{
    return incidents_.contains(incidentId);
}

void MailAlertManager::sendEmail(const std::string& subject, const std::string& body)
{
    if (dryRun_) {
        std::cerr << "[MailAlertManager DRY-RUN] Would send email:\n";
        std::cerr << "To: ";
        for (size_t i = 0; i < mailingList_.size(); ++i) {
            std::cerr << mailingList_[i];
            if (i + 1 < mailingList_.size()) std::cerr << ", ";
        }
        std::cerr << "\nSubject: " << subject << "\n\n"
                  << body << "\n"
                  << "----------------------------------------\n";
        return;
    }

    if (mailingList_.empty()) {
        logging::err("[MailAlertManager] Mailing list is empty, not sending.\n");
        return;
    }

    // poskladáme celý raw SMTP message (headers + body)
    std::ostringstream oss;
    oss << "From: " << gmailUser_ << "\r\n";
    oss << "To: ";
    for (size_t i = 0; i < mailingList_.size(); ++i) {
        oss << mailingList_[i];
        if (i + 1 < mailingList_.size()) oss << ", ";
    }
    oss << "\r\n";
    oss << "Subject: " << subject << "\r\n";
    oss << "Content-Type: text/plain; charset=\"utf-8\"\r\n";
    oss << "\r\n"; // koniec hlavičiek
    oss << body << "\r\n";

    std::string rawMessage = oss.str();

    if (!sendViaGmailSmtp(gmailUser_, gmailAppPassword_, gmailUser_, mailingList_, rawMessage)) {
        logging::err("[MailAlertManager] sendViaGmailSmtp failed.\n");
    }
}
