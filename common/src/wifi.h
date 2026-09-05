/*
 * wifi.h
 *
 *  Created on: Sep. 24, 2019
 *      Author: Danny (with help from copilot, Aug. 08, 2026)
 */

#ifndef WIFI_H_
#define WIFI_H_

#include <DNSServer.h>
#include "OTAHandler.h"
#include "WebHandlerBase.h"
#include "WebSocketBase.h"


class wifi {
public:
    wifi();

    void setup(WebHandlerBase *webhandler,
    		   WebSocketBase *sockethandler,
    		   const char *ssid_prefix,
			   const char *otapasswd);
    void loop();

    // Setter so the free function can notify us
    void notifySettingsChanged();

    // -------------------- Metrics Accessors --------------------
    uint32_t getStaConnectCount() const { return staConnectCount; }
    uint32_t getStaFailCount()    const { return staFailCount; }
    uint32_t getApFallbackCount() const { return apFallbackCount; }

    uint32_t getLastStaSuccess()  const { return lastStaSuccess; }
    uint32_t getLastStaFailure()  const { return lastStaFailure; }

    uint64_t getTotalStaUptime()  const { return totalStaUptime; }
    uint64_t getTotalApUptime()   const { return totalApUptime; }

private:
    // -------------------- Config --------------------
    static const uint16_t DNS_PORT;               // definition in .cpp
    static const uint16_t WIFI_CONNECT_TIMEOUT_MS = 15000;
    static const uint16_t WIFI_RETRY_DELAY_MS     = 3000;
    static const uint8_t  WIFI_FIELD_MAX_LEN      = 64;

    // -------------------- State Machine --------------------
    enum WifiState {
        WIFI_STATE_BOOT,
        WIFI_STATE_STA_CONNECTING,
        WIFI_STATE_STA_CONNECTED,
        WIFI_STATE_STA_FAILED,
        WIFI_STATE_AP_RUNNING,
        WIFI_STATE_STA_RECONNECTING
    };

    WifiState     state;
    unsigned long stateStartTime;
    unsigned long lastUptimeTick;
    unsigned long lastRetry;

    // -------------------- Flags --------------------
    bool otaInitialized;
    bool settingsChanged;

    // -------------------- Backoff --------------------
    uint32_t reconnectBackoff;

    // -------------------- Parameters --------------------
    const char *ssid_prefix;    // For AP SSID construction
    const char *ota_password;   // For OTA

    // -------------------- Modules --------------------
    DNSServer dns;
    OTAHandler ota;

    // -------------------- Metrics --------------------
    uint32_t staConnectCount;
    uint32_t staFailCount;
    uint32_t apFallbackCount;

    uint32_t lastStaSuccess;
    uint32_t lastStaFailure;

    uint64_t totalStaUptime;
    uint64_t totalApUptime;

    uint32_t lastStaStartTime;

    // -------------------- Internal Helpers --------------------
    void setState(WifiState s);
    const char *getStateName(WifiState s, char *code);
    void showState(WifiState s);
    bool elapsed(unsigned long ms);

    void startAP();
    void startSTA();
    void fsmTick();

    bool ssidVisible();   // scan-based STA retry
};

// Called by WebHandler when settings are saved
void wifi_settings_changed();


#endif /* WIFI_H_ */
