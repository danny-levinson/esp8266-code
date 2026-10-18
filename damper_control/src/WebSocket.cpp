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


void WebSocketApp::do_activatemanual(uint8_t clientnum) { // @suppress("Member declaration not found")
	StaticJsonDocument<100> jsonBuffer;     // .add fails silently if space is insufficient
	JsonObject root = jsonBuffer.createNestedObject();
	//root["seqNum"] = seqnum;
	root["activatemanual"] = "1";
	//SPRNTF(1, "jsonBuffer usage: %d\r\n", jsonBuffer.memoryUsage()); // last measured at 50
	String jsonout;
	serializeJson(root, jsonout);
	//SPRTLN(1, "sent bytes: "+String(jsonout.length())); // last measured at 31
	wsSend(clientnum, jsonout.c_str(), jsonout.length());
	g_manual = true;
	common_globals.saveDataToFile();
}

void WebSocketApp::do_activateauto(uint8_t clientnum) {
	StaticJsonDocument<100> jsonBuffer;     // .add fails silently if space is insufficient
	JsonObject root = jsonBuffer.createNestedObject();
	//root["seqNum"] = seqnum;
	root["activateauto"] = "1";
	//SPRNTF(1, "jsonBuffer usage: %d\r\n", jsonBuffer.memoryUsage()); // last measured at 50
	String jsonout;
	serializeJson(root, jsonout);
	//SPRTLN(1, "sent bytes: "+String(jsonout.length())); // last measured at 31
	wsSend(clientnum, jsonout.c_str(), jsonout.length());
	g_manual = false;
	common_globals.saveDataToFile();
}

void WebSocketApp::do_opendamper(uint8_t clientnum) {
	the_damper.open_damper();		// in damper.cpp
	StaticJsonDocument<100> jsonBuffer;     // .add fails silently if space is insufficient
	JsonObject root = jsonBuffer.createNestedObject();
	//root["seqNum"] = seqnum;
	root["opendamper"] = "1";
	//SPRNTF(1, "jsonBuffer usage: %d\r\n", jsonBuffer.memoryUsage()); // last measured at 50
	String jsonout;
	serializeJson(root, jsonout);
	//SPRTLN(1, "sent bytes: "+String(jsonout.length())); // last measured at 31
	wsSend(clientnum, jsonout.c_str(), jsonout.length());
	g_damperopen = true;
	common_globals.saveDataToFile();
}

void WebSocketApp::do_closedamper(uint8_t clientnum) {
	the_damper.close_damper();		// in damper.cpp
	StaticJsonDocument<100> jsonBuffer;     // .add fails silently if space is insufficient
	JsonObject root = jsonBuffer.createNestedObject();
	//root["seqNum"] = seqnum;
	root["closedamper"] = "1";
	//SPRNTF(1, "jsonBuffer usage: %d\r\n", jsonBuffer.memoryUsage()); // last measured at 50
	String jsonout;
	serializeJson(root, jsonout);
	//SPRTLN(1, "sent bytes: "+String(jsonout.length())); // last measured at 31
	wsSend(clientnum, jsonout.c_str(), jsonout.length());
	g_damperopen = false;
	common_globals.saveDataToFile();
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
