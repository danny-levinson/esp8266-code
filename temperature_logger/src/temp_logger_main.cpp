/*
 * Created in Sloeber, 2026-09-29 by Danny Levinson
 */


#include "StandardHelpers.h"
#include "WebHandler.h"
#include "WebSocket.h"

#include "app_code.h"

#define DEBUG 2			// override default of 1



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
 * 		hostnamepfx		- used for webhandler (settings page, debug page), AP station name (in wifi), OTA password
 * 		textname		- used only for serial port initial debug message
 * 		publicname		- set using settings page, shown there too, also used as wifi host name, ota login name
 */


// set TEST to 0 for the (a?) real controller
// set TEST to 1 for a chip connected to the USB port

#define TEST 1
#if TEST
const char* hostnamepfx = "AppCodeTest";						// used in AP mode to acquire publicname, OTA, etc
const char* textname = "App Code Test";		// device type, used for debug message to user
#else
const char* hostnamepfx = "TheApplication";
const char* textname = "The Application";
#endif


static StandardHelpers standardhelpers;

class WebHandler *my_webhandler = 0;
class WebSocketApp *my_socketHandler = 0;


void setup() {
    my_webhandler = new WebHandler(hostnamepfx);
    my_socketHandler = new WebSocketApp();
	standardhelpers.setup(my_webhandler, my_socketHandler, textname, hostnamepfx, false);
	the_app_code.setupAppCode();
}

void loop() {							// will have to add morse.update()
	standardhelpers.loopBegin();
	the_app_code.loopAppCode();
	standardhelpers.loopEnd();
}
