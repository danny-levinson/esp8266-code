/*
 * WebSocket.cpp
 *
 *  Created on: Aug. 20, 2026
 *      Author: Danny
 */


#include "WebSocket.h"

#include "SerialDebugHelper.h"
#include "ArduinoJson.h"

#include "FileSys.h"
#include "CommonGlobals.h"

#include "damper.h"


// definitions of global constants declared in CommonGlobals & set by ui; initialized from file system file

int g_manual, g_damperopen;

g_data_struct g_data_with_names[] = {
	&cg_offset,       "offset",
	&cg_tzone,        "tzone",
	&cg_dst,          "dst",
	&g_manual,		 "manual",
	&g_damperopen,	 "damperopen",
	NULL, NULL
};

void WebSocketApp::do_activatemanual(uint8_t clientnum) { // @suppress("Member declaration not found")
	respondToButton(clientnum, "activatemanual", "1", g_manual, true);
}

void WebSocketApp::do_activateauto(uint8_t clientnum) {
	respondToButton(clientnum, "activateauto", "1", g_manual, false);
}

void WebSocketApp::do_opendamper(uint8_t clientnum) {
	respondToButton(clientnum, "opendamper", "1", g_damperopen, true);
	the_damper.open_damper();		// in damper.cpp
}

void WebSocketApp::do_closedamper(uint8_t clientnum) {
	the_damper.close_damper();		// in damper.cpp
	respondToButton(clientnum, "closedamper", "1", g_damperopen, false);
}


WebSocketApp::WebSocketApp(uint16_t port) : WebSocketBase(port) {
}

void WebSocketApp::handleAppWebSocketMessage(uint8_t num,
                                             WStype_t type,
                                             String &payload)
{
	//String pfx = "["+String(num)+"] ";		// num is the websocket connection id if needed for feedback
    if (type != WStype_TEXT) {
    	SPRNTF(1, "message of unknown type '%d' ignored\n", type);
    	return;
	} else  if (strstr(payload.c_str(), "/activatemanual") != NULL) {
		SPRTLN(1, "manualactive was clicked");
		do_activatemanual(num);
	} else  if (strstr(payload.c_str(), "/activateauto") != NULL) {
		SPRTLN(1, "autoactive was clicked");
		do_activateauto(num);
	} else  if (strstr(payload.c_str(), "/opendamper") != NULL) {
		SPRTLN(1, "opendamper was clicked");
		do_opendamper(num);
	} else  if (strstr(payload.c_str(), "/closedamper") != NULL) {
		SPRTLN(1, "closedamper was clicked");
		do_closedamper(num);
	} else {
		SPRNTF(1, "bad socket request: %s\n", payload);
	}

/*	Examples of other messages we might want to handle:

    if (msg == "get_params") {
        wsSend(num, "params:mode=" + String(appMode) +
                     ",threshold=" + String(threshold));
    }

    if (msg.startsWith("set_mode=")) {
        appMode = msg.substring(9).toInt();
        wsSend(num, "mode updated");
    }

    if (msg.startsWith("set_threshold=")) {
        threshold = msg.substring(14).toInt();
        wsSend(num, "threshold updated");
    }
*/
}
