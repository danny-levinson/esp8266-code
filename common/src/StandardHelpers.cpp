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

/*
 * Sloeber version
 *
 * Compile using Sloeber|Verify
 * Load using Sloeber|Upload Sketch after setting port in Project|Properties|Sloeber
 * Note: for OTA loading, set OTA password to <hostnamepfx>+"Upd8" in Project|Properties|Sloeber|Port set password
 * - if you change the hostnamepfx, upload first with the old ota password, then change it to the new one
 * - OTA doesn't care about the login name, only the password
 *
 * Names:
 * 		nameasprefix	- used for webhandler (settings page, debug page), AP station name (in wifi), OTA password
 * 		title			- used only for serial port initial debug message
 * 		publicname		- set using settings page, shown there too, also used as wifi host name, ota login name
 */

//#define DEBUG 2			// override default of 1 for this file only (for illustration)

static wifi mywifi;


void StandardHelpers::setup(WebHandlerBase *webhandler,
							WebSocketBase *sockethandler,
							const char *title,
							const char *nameasprefix,
							bool debug) {
#if DEBUG
	Serial.begin(115200);
	Serial.setDebugOutput(debug);		// turn on/off wifi debug messages (alt: disable debug port in project|properties)
	SPRTLN(1, "");
	SPRTLN(1, title);
	SPRTLN(1, "");
	init_timings();
#endif
	setup_file_system();
	CrashCheckpoint_init();

	static char otapasswd[40];
	strncpy(otapasswd, nameasprefix, 39);
	otapasswd[39] = '\0';
	strncat(otapasswd, "Upd8", 39 - strlen(otapasswd));
	mywifi.setup(webhandler, sockethandler, nameasprefix, otapasswd);
}

void StandardHelpers::loopBegin() {
	mywifi.loop();
}

void StandardHelpers::loopEnd() {
	MorseSignaller::update();
	record_time_diff();
	delay(get_delay_time());
}
