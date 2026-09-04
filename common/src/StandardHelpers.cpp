/*
 * File containing setup & loop to run normal debugging, etc, functions
 * the before app specific code
 */

#include "StandardHelpers.h"

#include "MorseSender.h"
#include "DelayTimings.h"
#include "FileSys.h"
#include "SerialDebugHelper.h"
#include "wifi.h"
#include "WebHandlerBase.h"
#include "CrashLog.h"

#define DEBUG 2			// override default of 1 for this file only (for illustration)

static wifi mywifi;


void StandardHelpers::setup(WebHandlerBase *webhandler,
							WebSocketBase *sockethandler,
							const char *nameaspfx,
							bool debug) {
#if DEBUG
	Serial.begin(115200);
	Serial.setDebugOutput(debug);		// turn on/off wifi debug messages (alt: disable debug port in project|properties)
	SPRTLN(1, "");
	SPRTLN(1, nameaspfx);
	SPRTLN(1, "");
	init_timings();
#endif
	setup_file_system();
	CrashLog::begin();
	CrashInfo ci;
	if (CrashLog::load(ci)) {
		SPRNTF(1, "Last boot reason: %s, %u, %u\n", ci.reason.c_str(), ci.lastState, ci.uptime);
	}

	static char otapasswd[40];
	strncpy(otapasswd, nameaspfx, 39);
	otapasswd[39] = '\0';
	strncat(otapasswd, "Upd8", 39 - strlen(otapasswd));
	mywifi.setup(webhandler, sockethandler, nameaspfx, otapasswd);
}

void StandardHelpers::loopBegin() {
	mywifi.loop();
}

void StandardHelpers::loopEnd() {
	MorseSignaller::update();
	record_time_diff();
	delay(get_delay_time());
}
