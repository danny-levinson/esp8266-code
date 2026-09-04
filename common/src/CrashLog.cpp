/*
 * CrashLog.cpp
 *
 *  Created on: Sep. 3, 2026
 *      Author: Danny
 */


/* integrations:
 *
- in setup():

CrashLog::begin();

CrashInfo ci;
if (CrashLog::load(ci)) {
    // send crash report, log it, expose via WebHandler
    CrashLog::clear();
}

- in FSM transitions:

CrashLog::recordState(currentState);

- in WebHandlerBase:

void WebHandler::handleCrashReport(ESP8266WebServer &server) {
    CrashInfo ci;
    if (!CrashLog::load(ci)) {
        server.send(200, "application/json", "{\"crash\":false}");
        return;
    }
    char buf[256];
    snprintf(buf, sizeof(buf),
        "{\"crash\":true,\"reason\":%u,\"epc1\":%u,\"excvaddr\":%u,"
        "\"depc\":%u,\"lastState\":%u,\"uptime\":%u}",
        ci.reason, ci.epc1, ci.excvaddr, ci.depc, ci.lastState, ci.uptime);

    server.send(200, "application/json", buf);

- and to register handler:

server.on("/crash", HTTP_GET, [&](){ handleCrashReport(server); });


}


 *
 */

#include "CrashLog.h"
#include "FileSys.h"


uint32_t CrashLog::lastState = 0;
static const char *tmplogname = "/crash.log";


void CrashLog::begin() {
    //setup_file_system();	//FileSys.begin();

    String reason = ESP.getResetReason();

    // Maybe detect crash-like resets
    if (/*reason != "Power On" &&
        reason != "External System" &&
        reason != "Software/System restart"*/ 1) {

        CrashInfo ci;
        ci.reason    = reason;
        ci.lastState = lastState;
        ci.uptime    = millis();   // uptime before crash is unknown

        write(ci);
        rotateLogs();
    }
}

void CrashLog::rotateLogs() {
	char namebuf1[20], namebuf2[20];
	const char *patt = "/crash-%d.log";
	sprintf(namebuf1, patt, 9);
	if (FileSys.exists(namebuf1)) {
		FileSys.remove(namebuf1);
	}
	for (int i=8; i>=0; i--) {
		sprintf(namebuf1, "/crash-%d.log", i);
		if (FileSys.exists(namebuf1)) {
			sprintf(namebuf2, patt, i+1);
			FileSys.rename(namebuf1, namebuf2);
		}
	}
	FileSys.rename(tmplogname, namebuf1);
}

void CrashLog::recordState(uint32_t state) {
    lastState = state;
}

void CrashLog::write(const CrashInfo &ci) {
    File f = FileSys.open(tmplogname, "w");
    if (!f) return;

    f.printf("%s,%u,%u\n",
        ci.reason.c_str(),
        ci.lastState,
        ci.uptime
    );
    f.close();
}

bool CrashLog::load(CrashInfo &ci) {
    File f = FileSys.open("/crash-0.log", "r");
    if (!f) return false;

    String line = f.readStringUntil('\n');
    f.close();

    if (line.length() == 0) return false;

    int p1 = line.indexOf(',');
    int p2 = line.indexOf(',', p1 + 1);

    ci.reason    = line.substring(0, p1);
    ci.lastState = line.substring(p1 + 1, p2).toInt();
    ci.uptime    = line.substring(p2 + 1).toInt();

    return true;
}

void CrashLog::clear() {
    FileSys.remove(tmplogname);
}
