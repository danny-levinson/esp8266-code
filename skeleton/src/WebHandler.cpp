/*
 * WebHandler.cpp
 *
 *  Created on: Sep. 29, 2026
 *      Author: Danny
 */


#include "WebHandler.h"

#include "DataSaver.h"


WebHandler::WebHandler(const char *genericname) :
	WebHandlerBase(genericname)
{
	appDataSaver = new DataSaver(&appData, sizeof(appData), "/appconfig");
}


bool WebHandler::saveAppData() {
	//the_app_code.get_angles(appData.ang1, appData.ang2);
	return appDataSaver->write();
}

bool WebHandler::restoreAppData() {
	bool r = appDataSaver->read();
	//if (r) the_app_code.set_angles(appData.ang1, appData.ang2);
	return r;
}



void WebHandler::registerAppRoutes()
{
	//httpServer.on("/dbgsetservoangles", HTTP_GET, [this]() { this->debug_set_servo_angles(); });
}


//static char debug_page_form_extra[] PROGMEM = R"=====(
//<hr/>
//<form method=GET action="dbgsetservoangles">
//Servo angles -
//)=====";
//
//static char debug_page_form_ender[] PROGMEM = R"=====(
//  <input class="button" type="submit" value="Set" />
//</form>
//)=====";

void WebHandler::renderAppDebugInfo()
{
	WiFiClient client = httpServer.client();
//	CP(FPSTR(debug_page_form_extra));
//	int ang1, ang2;
//	the_damper.get_servo_angles(ang1, ang2);
//	client.printf("Closed: <input type=\"text\" name=\"ang1\" size=\"4\" value=\"%d\">\r\n", ang1);
//	client.printf("Open: <input type=\"text\" name=\"ang2\" size=\"4\" value=\"%d\">\r\n", ang2);
//	CP(FPSTR(debug_page_form_ender));
}

void WebHandler::handleAppSettings()
{

}
