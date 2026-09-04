/*
 * wifi.cpp
 *
 *  Created on: Sep. 24, 2019
 *      Author: Danny
 *
 *      Functionality
 *      	sets up wifi as station or access point
 *      	acquires net credentials as needed
 *      	calls ota & webhandler to do work from setup & loop
 *
 *      History:
 *      	2024-10-02
 *      		converted from spiffs to virtual FileSys
 *      		setting.h, debug.h -> NetSettings.h, SerialDebug.h
 *      		dropped formatBytes(), setup_spiffs() [latter is now in NetSettings]
 *      		NetSettings structured as a class now
 *      		reinstated debug statements
 *      	2024-11-27
 *      		WebHandler structures as class now
 *      		more functionality (start_up_station, configure_as_access_point) incorporated into class wifi
 *      		wifi_connection_status states added
 *      		tried to reconfigure as non-blocking
 *      	2024-12-02
 *      		try_to_reconnect() moved down
 *      		more work on making it non-blocking
 *      	2025-02-06
 *      		renamed functions to start_up_as_station, start_up_as_access_point
 *      	2026-08-06
 *      		rewritten by copilot to not block and reconciled with old wifi & framework
 */

#include <ESP8266WiFi.h>
#include "NetSettings.h"

#include "wifi.h"

#include "SerialDebugHelper.h"
#include "MorseSender.h"
#include "CrashLog.h"
#include "WebHandlerBase.h"

/*
States:
        WIFI_STATE_BOOT					initial state - only used once
        WIFI_STATE_STA_CONNECTING,
        WIFI_STATE_STA_CONNECTED,
        WIFI_STATE_STA_FAILED,
        WIFI_STATE_AP_RUNNING,
        WIFI_STATE_STA_RECONNECTING
*/

// Required definition for static const member
const uint16_t wifi::DNS_PORT = 53;

static wifi *wifi_instance = nullptr;
static MorseSignaller *morse_signaller = 0;


class WebHandlerBase *the_webhandler = 0;


// -------------------- Callback from WebHandler --------------------
void wifi_settings_changed() {
    if (wifi_instance) {
        wifi_instance->notifySettingsChanged();
    }
}

// -------------------- Constructor --------------------
wifi::wifi() :
    state(WIFI_STATE_BOOT),
    otaInitialized(false),
    settingsChanged(false),
    lastUptimeTick(0),
    lastRetry(0),
	stateStartTime(0),
    reconnectBackoff(WIFI_RETRY_DELAY_MS),
    ssid_prefix(""),
    ota_password(""),
    staConnectCount(0),
    staFailCount(0),
    apFallbackCount(0),
    lastStaSuccess(0),
    lastStaFailure(0),
    totalStaUptime(0),
    totalApUptime(0),
	lastStaStartTime(0)
{
    wifi_instance = this;
}

// -------------------- Public API --------------------
void wifi::setup(WebHandlerBase *webhandler,
				 WebSocketBase *sockethandler,
				 const char *ssid_prefix_param,
				 const char *otapasswd)
	{
    ssid_prefix = ssid_prefix_param;
    ota_password = otapasswd;

    morse_signaller = new MorseSignaller(1);		// on channel 1

	setState(WIFI_STATE_BOOT);
	net_settings = new NetSettings();

    // Read client name once for WebHandler construction
    char client_name[WIFI_FIELD_MAX_LEN + 1];
    char ssid[WIFI_FIELD_MAX_LEN + 1];
    char pass[WIFI_FIELD_MAX_LEN + 1];

    if (net_settings && net_settings->settingsExist()) {
        net_settings->read(WIFI_FIELD_MAX_LEN, client_name, ssid, pass);
        SPRTLN(1, "read net settings in setup");
    } else {
        client_name[0] = '\0';
    }
    if (client_name[0] != '\0') {
    	if (! WiFi.hostname(client_name)) SPRNTF(1, "WiFi.hostname() failed (%s)\r\n", client_name);
    	SPRNTF(1, "hostname set to %s in setup\r\n", client_name);
    }

    the_webhandler = webhandler;
    the_webhandler->setWifi(wifi_instance);
    the_webhandler->setSocketHandler(sockethandler);
}

void wifi::loop() {
    fsmTick();
}

void wifi::notifySettingsChanged() {
    settingsChanged = true;
}

// -------------------- Debugging & Monitoring --------------------

void wifi::showState(WifiState state) {
	struct {
		WifiState st;
		char *stname;
		char stcode;
	} state_table[]= {
			{WIFI_STATE_BOOT, "boot", 'a'},
			{WIFI_STATE_STA_CONNECTING, "sta_connecting", 'b'},
			{WIFI_STATE_AP_RUNNING, "ap_running", 'c'},
			{WIFI_STATE_STA_FAILED, "sta_failed", 'd'},
			{WIFI_STATE_STA_CONNECTED, "sta_connected", 'e'},
			{WIFI_STATE_STA_RECONNECTING, "sta_reconnecting", 'f'},
			{WIFI_STATE_BOOT, 0, 'g'}
	};
	static WifiState laststate = WIFI_STATE_STA_FAILED;		// initialise to anything but boot
	static int ditto_count = 0, ditto_limit = 500;
	static char *lastname = "xxx";
	if (state != laststate) {								// should always be true
		char *nm = 0, newcode = 'x';
		for (int i = 0; state_table[i].stname != 0; i++) {
			if (state_table[i].st == state) {
				nm = state_table[i].stname;
				newcode = state_table[i].stcode;
			}
		}
		if (nm == 0) nm = "unknown";
		SPRNTF(1, "state: %s\r\n", nm);
		morse_signaller->signal(newcode);
		laststate = state;
		lastname = nm;
		ditto_count = 0;
		ditto_limit = 500;
		return;
	}
	if (++ditto_count % ditto_limit == 0) {
		if (ditto_limit < 3000) ditto_limit *= 2;
		SPRNTF(1, "state: %s (%d)\r\n", lastname, ditto_count);
	}
}

// -------------------- Internal Helpers --------------------
void wifi::setState(WifiState newstate) {
	if (newstate != state) {
		state = newstate;
		lastUptimeTick = stateStartTime = millis();
		CrashLog::recordState((uint32_t)newstate);
		showState(newstate);
	}
}

bool wifi::elapsed(unsigned long ms) {
    return (millis() - stateStartTime) >= ms;
}

// -------------------- AP Mode --------------------
void wifi::startAP() {
    WiFi.mode(WIFI_AP);

    // Get MAC address
    uint8_t mac[6];
    WiFi.macAddress(mac);

    // Build SSID: <ssid_prefix>-XXYYZZ
    char ap_ssid[64];
    snprintf(ap_ssid, sizeof(ap_ssid),
             "%s-%02X%02X%02X",
             ssid_prefix,
             mac[3], mac[4], mac[5]);

    // Start AP

    // REQUIRED: ensure AP interface has correct IP/netmask/gateway
    WiFi.softAPConfig(
        IPAddress(192,168,4,1),
        IPAddress(192,168,4,1),
        IPAddress(255,255,255,0)
    );
    WiFi.softAP(ap_ssid, "configure123", 1);

    // Start DNS server
    SPRTLN(2, "starting dns server");
    IPAddress apIP = WiFi.softAPIP();
    dns.start(DNS_PORT, "*", apIP);

    the_webhandler->setupWebhandling(false);  // AP mode
    setState(WIFI_STATE_AP_RUNNING);
}

// -------------------- STA Mode --------------------
void wifi::startSTA() {
    // If no settings exist → go straight to AP mode
    if (!net_settings || !net_settings->settingsExist()) {
    	SPRTLN(1, "no net settings in startSTA()");
        startAP();
        return;
    }

    // Settings exist → read them
    SPRTLN(3, "have net settings in startSTA()");
    char client_name[WIFI_FIELD_MAX_LEN + 1];
    char ssid[WIFI_FIELD_MAX_LEN + 1];
    char pass[WIFI_FIELD_MAX_LEN + 1];

    SPRTLN(3, "reading net settings");
    net_settings->read(WIFI_FIELD_MAX_LEN, client_name, ssid, pass);
    SPRTLN(1, "read net settings in startSTA");

    // Begin STA mode
    if (! WiFi.hostname().equals(client_name)) {
    	if (! WiFi.hostname(client_name)) SPRNTF(1, "WiFi.hostname() failed (%s)\r\n", client_name);
    	SPRNTF(1, "hostname reset to %s\r\n", client_name);
    }
    if (! WiFi.mode(WIFI_STA)) SPRTLN(1, "WiFi.mode() failed");
    WiFi.begin(ssid, pass);
    lastStaStartTime = millis();

    SPRTLN(1, "started STA mode, setting up webhandling");
    the_webhandler->setupWebhandling(true);   // STA mode

    // Transition to STA_CONNECTING
    setState(WIFI_STATE_STA_CONNECTING);
}

// -------------------- Scan for SSID (for AP-mode retry) --------------------
bool wifi::ssidVisible() {
    char client_name[WIFI_FIELD_MAX_LEN + 1];
    char ssid[WIFI_FIELD_MAX_LEN + 1];
    char pass[WIFI_FIELD_MAX_LEN + 1];

    if (!(net_settings && net_settings->settingsExist())) {
        return false;
    }

    net_settings->read(WIFI_FIELD_MAX_LEN, client_name, ssid, pass);

    int n = WiFi.scanNetworks(false, false);
    for (int i = 0; i < n; i++) {
        if (WiFi.SSID(i) == ssid) { // @suppress("Invalid arguments")
            return true;
        }
    }
    return false;
}

// -------------------- FSM Core --------------------
void wifi::fsmTick() {
    // Always run WebHandler
    the_webhandler->loopWebhandling();

    // Continuous uptime accumulation
    unsigned long now = millis();
    unsigned long duration = now - lastUptimeTick;

    switch (state) {
        case WIFI_STATE_STA_CONNECTED:
            totalStaUptime += duration;
            break;

        case WIFI_STATE_AP_RUNNING:
            totalApUptime += duration;
            break;

        default:
            break;
    }
    lastUptimeTick = now;   // reset for next tick

    switch (state) {

    case WIFI_STATE_BOOT:
        startSTA();   // may transition to AP mode internally
        break;

    case WIFI_STATE_STA_CONNECTING:
        if (WiFi.status() == WL_CONNECTED) {
            staConnectCount++;
            lastStaSuccess = millis();
            otaInitialized = false;
            reconnectBackoff = WIFI_RETRY_DELAY_MS;
            SPRNTF(1, "connected! after %f seconds\n", (millis() - lastStaStartTime) / 1000.0);
            setState(WIFI_STATE_STA_CONNECTED);
        } else if (elapsed(WIFI_CONNECT_TIMEOUT_MS)) {
            // Initial STA failed → go to AP
        	SPRNTF(1, "connect failed after %f seconds\n", (millis() - lastStaStartTime) / 1000.0);
            staFailCount++;
            lastStaFailure = millis();
            reconnectBackoff = WIFI_RETRY_DELAY_MS;
            lastRetry = millis();
            setState(WIFI_STATE_STA_FAILED);
        }
        break;

    case WIFI_STATE_STA_CONNECTED:
        if (!otaInitialized) {
            char client_name[WIFI_FIELD_MAX_LEN + 1];
            char ssid[WIFI_FIELD_MAX_LEN + 1];
            char pass[WIFI_FIELD_MAX_LEN + 1];

            if (net_settings && net_settings->settingsExist()) {
                net_settings->read(WIFI_FIELD_MAX_LEN, client_name, ssid, pass);
            } else {
                client_name[0] = '\0';
            }

            ota.setup(client_name, ota_password);
            otaInitialized = true;
        }
        ota.loop();

        if (WiFi.status() != WL_CONNECTED) {
            staFailCount++;
            lastStaFailure = millis();
            reconnectBackoff = WIFI_RETRY_DELAY_MS;
            lastRetry = millis();
            setState(WIFI_STATE_STA_FAILED);
        }
        break;

    case WIFI_STATE_STA_FAILED:
        // Retry STA before falling back to AP mode
    	SPRNTF(1, "fallback - time since last retry: %d, reconnectBackoff: %d\r\n", millis()-lastRetry, reconnectBackoff);
        if ((millis() - lastRetry) <= reconnectBackoff) {
            lastRetry = millis();
            WiFi.disconnect();						// Try STA again
            startSTA();
            reconnectBackoff *= 2;					// Exponential backoff with jitter
            if (reconnectBackoff > 60000U) {
                reconnectBackoff = 60000U;
            }
            reconnectBackoff += random(0, 250);
            setState(WIFI_STATE_STA_RECONNECTING);	// Overrides WIFI_STATE_STA_CONNECTING set by startSTA()
            break;
        }
        // If retry delay exceeded → fall back to AP mode
        apFallbackCount++;     						// metric
        startAP();
        //setState(WIFI_STATE_AP_RUNNING);
        break;

    case WIFI_STATE_AP_RUNNING:
        dns.processNextRequest();

        // User changed settings → immediate reconnect attempt
        if (settingsChanged) {
            settingsChanged = false;

            WiFi.softAPdisconnect(true);
            dns.stop();

            startSTA();
            lastRetry = millis();
            reconnectBackoff = WIFI_RETRY_DELAY_MS;
            setState(WIFI_STATE_STA_RECONNECTING);
            break;
        }

        // NEW: automatic STA retry with scan + backoff
        if ((millis() - lastRetry) >= reconnectBackoff) {
            lastRetry = millis();

            if (ssidVisible()) {
                WiFi.softAPdisconnect(true);
                dns.stop();

                startSTA();
                reconnectBackoff = WIFI_RETRY_DELAY_MS;
                setState(WIFI_STATE_STA_RECONNECTING);
            } else {
                // SSID not visible → exponential backoff with jitter
                reconnectBackoff *= 2;
                if (reconnectBackoff > 60000U) {
                    reconnectBackoff = 60000U;
                }
                reconnectBackoff += random(0, 250);
            }
        }
        break;

    case WIFI_STATE_STA_RECONNECTING:
        if (WiFi.status() == WL_CONNECTED) {
            staConnectCount++;
            lastStaSuccess = millis();
            otaInitialized = false;
            reconnectBackoff = WIFI_RETRY_DELAY_MS;
            SPRNTF(1, "reconnected! after %f seconds\n", (millis() - lastStaStartTime) / 1000.0);
            setState(WIFI_STATE_STA_CONNECTED);
            break;
        }

        if (elapsed(WIFI_CONNECT_TIMEOUT_MS)) {
            // STA reconnect failed → back to AP
            apFallbackCount++;
            reconnectBackoff = WIFI_RETRY_DELAY_MS;
            startAP();
            break;
        }

        // Periodic retry with backoff
        if ((millis() - lastRetry) >= reconnectBackoff) {
            lastRetry = millis();

            WiFi.disconnect();
            startSTA();

            // Exponential backoff with jitter
            reconnectBackoff *= 2;
            if (reconnectBackoff > 60000U) {
                reconnectBackoff = 60000U;  // cap at 60 seconds
            }
            reconnectBackoff += random(0, 250);
        }
        break;


    default:
        setState(WIFI_STATE_BOOT);
        break;
    }
}
