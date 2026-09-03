/*
 * OTAHandler.cpp
 *
 *  Created on: Sep. 10, 2019
 *      Author: Danny
 */

#include <Arduino.h>
#include <ArduinoOTA.h>
#include "OTAHandler.h"

#include "SerialDebugHelper.h"


OTAHandler::OTAHandler() {
}

void OTAHandler::setup(const char *publicname, const char *otapasswd) {
	  ArduinoOTA.setHostname(publicname);
	  ArduinoOTA.setPassword(otapasswd);
	  ArduinoOTA.onStart([]() {
		SPRTLN(1,"Start OTA");
	  });
	  ArduinoOTA.onEnd([]() {
		SPRTLN(1,"\nEnd OTA");
	  });
	  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
		SPRNTF(1,"Progress: %u%%\r", (progress / (total / 100)));
	  });
	  ArduinoOTA.onError([](ota_error_t error) {
		SPRNTF(1,"Error[%u]: ", error);
		if (error == OTA_AUTH_ERROR) { SPRTLN(1,"Auth Failed"); }
		else if (error == OTA_BEGIN_ERROR) { SPRTLN(1,"Begin Failed"); }
		else if (error == OTA_CONNECT_ERROR) { SPRTLN(1,"Connect Failed"); }
		else if (error == OTA_RECEIVE_ERROR) { SPRTLN(1,"Receive Failed"); }
		else if (error == OTA_END_ERROR) { SPRTLN(1,"End Failed"); }
	  });
	  ArduinoOTA.begin();
	  SPRTLN(1,"OTA ready");
}

void OTAHandler::loop() {
	ArduinoOTA.handle();
}



