/*
 * WebSocket.cpp
 *
 *  Created on: Sep. 29, 2026
 *      Author: Danny
 */


#include "WebSocket.h"

#include "SerialDebugHelper.h"
#include "ArduinoJson.h"

#include "FileSys.h"
#include "CommonGlobals.h"


// definitions of global constants declared in CommonGlobals & set by ui; initialized from file system file

int g_manual, g_damperopen;

g_data_struct g_data_with_names[] = {
	&cg_offset,       "offset",
	&cg_tzone,        "tzone",
	&cg_dst,          "dst",
	&g_manual,		 "manual",		// for illustration
	NULL, NULL
};


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
