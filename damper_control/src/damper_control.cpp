/*
 * Created in Sloeber, 2021-10-08 by Danny Levinson
 */

// Do not remove the include below
#include "damper_control.h"

#include "StandardHelpers.h"
#include "wifi.h"
#include "DelayTimings.h"
#include "FileSys.h"
#include "damper.h"
#define DEBUG 2			// override default of 1

#include "WebHandler.h"
#include "WebSocket.h"


/*
 * Sloeber version
 *
 * Compile using build
 * Load using launch if connected directly,
 * or using 'loadprog' from command line if OTA
 *
 * See initial instructions above start_up_station() in wifi.cpp to establish initial connection
 */

//static const int ledPin = D4;              // for Wemos D1 mini

// set TEST to 0 for the (a?) real controller
// set TEST to 1 for a chip connected to the USB port

#define TEST 1
#if TEST
const char* hostnamepfx = "DamperControlTest";		// used only in AP mode to acquire publicname, etc
const char* textname = "Damper Controller Test";			// device type, used for messages to user
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
	standardhelpers.setup(my_webhandler, my_socketHandler, textname, false);
	the_damper.setup_damper();
}

void loop() {							// will have to add morse.update()
	standardhelpers.loopBegin();
	the_damper.loop_damper();
	standardhelpers.loopEnd();
}
