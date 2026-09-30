/*
 * Created in Sloeber, 2021-10-08 by Danny Levinson
 */


#include "StandardHelpers.h"
#include "WebHandler.h"
#include "WebSocket.h"

#include "damper.h"

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
const char* hostnamepfx = "DCtest";						// used in AP mode to acquire publicname, OTA, etc
const char* textname = "Damper Controller Test";		// device type, used for debug message to user
#else
const char* hostnamepfx = "DamperControl";
const char* textname = "Damper Controller";
#endif


static StandardHelpers standardhelpers;

class WebHandler *my_webhandler = 0;
class WebSocketApp *my_socketHandler = 0;


void setup() {
    my_webhandler = new WebHandler(hostnamepfx);
    my_socketHandler = new WebSocketApp();
	standardhelpers.setup(my_webhandler, my_socketHandler, textname, hostnamepfx, false);
	the_damper.setup_damper();
}

void loop() {							// will have to add morse.update()
	standardhelpers.loopBegin();
	the_damper.loop_damper();
	standardhelpers.loopEnd();
}
