/*
 * Created in Sloeber, 2026-09-29 by Danny Levinson
 */


#include "StandardHelpers.h"
#include "temperature_logger.h"
#include "WebHandler.h"
#include "WebSocket.h"


/*
 * Functions of this temperature logger application:
 * - read temperature from the peripheral at regular intervals and save in memory
 * - when enough temperatures have been read, save them to a file
 * - on demand, deliver files, or info about files to a client over local internet
 * - on demand, deliver data for drawing a temperature graph
 * - establish communication with a switch (e.g. for a fan or heater)
 * - according to preset limits, send on/off commands to the switch
 * - allow for changes to the preset limits
 */


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
const char* hostnamepfx = "TempLogTest";						// used in AP mode to acquire publicname, OTA, etc
const char* textname = "Temperature Logger Test";		// device type, used for debug message to user
#else
const char* hostnamepfx = "TemperatureLogger";
const char* textname = "Temperature Logger";
#endif


static StandardHelpers standardhelpers;

class WebHandler *my_webhandler = 0;
class WebSocketApp *my_socketHandler = 0;


void setup() {
    my_webhandler = new WebHandler(hostnamepfx);
    my_socketHandler = new WebSocketApp();
	standardhelpers.setup(my_webhandler, my_socketHandler, textname, hostnamepfx, false);
	the_temperature_logger.setupTemperatureLogger();					// also sets up sensor handling
}

void loop() {							// will have to add morse.update()
	standardhelpers.loopBegin();
	the_temperature_logger.loopTemperatureLogger();
	standardhelpers.loopEnd();
}
