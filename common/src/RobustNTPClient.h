#ifndef ROBUST_NTP_CLIENT_H
#define ROBUST_NTP_CLIENT_H

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

class RobustNTPClient {
public:
    RobustNTPClient(const char *server1,
                    const char *server2 = nullptr,
                    uint16_t port = 123);

    void begin();          // start UDP
    bool update();         // non-blocking sync attempt
    bool isReady() const;  // true after first successful sync
    uint32_t getEpoch();   // seconds since Jan 1 1970

private:
    bool dnsReady() const;
    bool resolveServer(IPAddress &ip);
    bool sendRequest(const IPAddress &ip);
    bool readResponse(uint32_t &epoch);

    const char *server1;
    const char *server2;
    uint16_t port;

    WiFiUDP udp;

    bool ready;
    uint32_t lastEpoch;
    uint32_t lastSync;

    // Backoff scheduling
    uint32_t nextAttempt;
    uint32_t backoff;

    // Request/response tracking
    bool waitingForResponse;
    uint32_t requestSent;

    // Server alternation
    bool usePrimary;
};

#endif
