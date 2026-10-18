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



void WebSocketApp::sendInitData(uint8_t clientnum) {
	StaticJsonDocument<1500> jsonBuffer;     // .add fails silently if space is insufficient
	JsonObject root = jsonBuffer.createNestedObject();
	JsonObject data = jsonBuffer.createNestedObject();
	int i;
	if (common_globals.dataWasInitialised()) {
		for (i=0; g_data_with_names[i].gvar != NULL; i++) {
			data[g_data_with_names[i].uiname] = *g_data_with_names[i].gvar;
		}
	}
	root["initData"] = data;
	//SPRNTF(1, "jsonBuffer usage: %d for %d items\r\n", jsonBuffer.memoryUsage(), i); // last measured at 1034
	String jsonout;
	serializeJson(root, jsonout);
	//SPRTLN(1, "json string: "+jsonout);
	SPRTLN(1, "sent bytes: "+String(jsonout.length())); // last measured at 261
	wsSend(clientnum, jsonout.c_str(), jsonout.length());
}





WebSocketApp::WebSocketApp(uint16_t port) : WebSocketBase(port) {
}

void WebSocketApp::handleAppWebSocketMessage(uint8_t num,
                                             WStype_t type,
                                             String &payload)
{
	String pfx = "["+String(num)+"] ";		// num is the websocket connection id
    if (type != WStype_TEXT) return;
	if (strstr(payload.c_str(), "/initialise") != NULL) {
		//SPRTLN(1, pfx+"initialising: "+(char*)payload);
		sendInitData(num);
	} else  if (strstr(payload.c_str(), "/change?") != NULL) {				// handle changes to global (gdata) variables
		SPRTLN(1, pfx+"change: "+payload.c_str());
		char *varname; int varval, radio;
		parseSocketChangePayload(payload.c_str(), &varname, varval, radio);
		if (varname == NULL) { SPRTLN(1, "payload not found"); }
		else common_globals.setVarByName(varname, varval, radio);
	} else  if (strstr(payload.c_str(), "/demo") != NULL) {
		common_globals.showGVars();
		//do_demo();
	} else {
		SPRTLN(1, "bad socket request: "+payload);
	}

/*
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
