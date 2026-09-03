/*
 * WebHandler.cpp
 *
 *  Created on: Aug. 18, 2026
 *      Author: Danny
 */


#include "WebHandler.h"

#include "DataSaver.h"

#include "config.h"
#include "damper.h"


WebHandler::WebHandler(const char *genericname) :
	WebHandlerBase(genericname)
{
	appDataSaver = new DataSaver(&appData, sizeof(appData), "/appconfig");
}


bool WebHandler::saveAppData() {
	the_damper.get_servo_angles(appData.ang1, appData.ang2);
	return appDataSaver->write();
}

bool WebHandler::restoreAppData() {
	bool r = appDataSaver->read();
	if (r) the_damper.set_servo_angles(appData.ang1, appData.ang2);
	return r;
}


#define CP(s) client.print(s)
#define CPN(s) client.println(s)
#define CPF(s) client.print(F(s))
#define CPNF(s) client.println(F(s))

void WebHandler::debug_set_servo_angles() {
	int ang1, ang2;
	bool good = true;
	if (! httpServer.hasArg("ang1")) good = false;
	else ang1 = atoi(httpServer.arg("ang1").c_str());
	if (! httpServer.hasArg("ang2")) good = false;
	else ang2 = atoi(httpServer.arg("ang2").c_str());
	WiFiClient client = httpServer.client();
	CPNF("HTTP/1.1 200 OK");
	CPNF("Content-Type: text/html");
	CPNF("Connection: close");
	CPN();
	if (! good) {
		//Serial.println("failed to parse angle for setting");
		client.println("failed setting angles");
		return;
	}
	//Serial.printf("setting angles: ang1: %d, ang2: %d\r\n", ang1, ang2);
	client.printf("setting angles: ang1: %d, ang2: %d\r\n", ang1, ang2);
	the_damper.set_servo_angles(ang1, ang2);
	saveAppData();
}

void WebHandler::registerAppRoutes()
{
	httpServer.on("/dbgsetservoangles", HTTP_GET, [this]() { this->debug_set_servo_angles(); });
}


static char debug_page_form_extra[] PROGMEM = R"=====(
<hr/>
<form method=GET action="dbgsetservoangles">
Servo angles - 
)=====";

static char debug_page_form_ender[] PROGMEM = R"=====(
  <input class="button" type="submit" value="Set" />
</form>
)=====";

void WebHandler::renderAppDebugInfo()
{
	WiFiClient client = httpServer.client();
	CP(FPSTR(debug_page_form_extra));
	int ang1, ang2;
	the_damper.get_servo_angles(ang1, ang2);
	client.printf("Closed: <input type=\"text\" name=\"ang1\" size=\"4\" value=\"%d\">\r\n", ang1);
	client.printf("Open: <input type=\"text\" name=\"ang2\" size=\"4\" value=\"%d\">\r\n", ang2);
	CP(FPSTR(debug_page_form_ender));
}

void WebHandler::handleAppSettings()
{

}
