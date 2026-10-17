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
	&g_manual,		 "manual",
	&g_damperopen,	 "damperopen",
	NULL, NULL
};

//extern int real_demolevel;


// Special handling for radio buttons as we haven't figured out how
// to make javascript/jquery trigger events for consequent changes.
// NOTE: for this to work, radio button id's must be of the form 'xx..xd'
// where the last character is the only thing that changes between
// buttons in the group.

void WebSocketApp::setVarByName(char *varname, int varval, int radio) {
	for (int i=0; g_data_with_names[i].gvar != NULL; i++) {
		if (strcmp(varname, g_data_with_names[i].uiname) == 0) {
			*(g_data_with_names[i].gvar) = varval;
			SPRTLN(1, String("set ")+varname+" to "+String(varval));
			if (radio) {
				for (int j=0; g_data_with_names[j].gvar != NULL; j++) {
					int n = strlen(varname) - 1;
					if (j != i && strncmp(varname, g_data_with_names[j].uiname, n) == 0) {
						*(g_data_with_names[j].gvar) = 0;
						SPRTLN(1, String("set ")+g_data_with_names[j].uiname+" to 0");
					}
				}
			}
			common_globals.saveDataToFile();
			return;
		}
	}
	SPRTLN(1, String("gvar matching '")+varname+"' not found");
}



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




static char *getStrSocketPayloadParam(const char *payload, char *param, char *outstr, int outlen) {
  char pstr[40];
  sprintf(pstr, "?%s=", param);
  char *p = strstr((char*)payload, pstr);
  if (p == NULL) {
    sprintf(pstr, "&%s=", param);
    p = strstr(payload, pstr);
  }
  if (p == NULL) return 0;
  p += strlen(param) + 2;
  char *q = strchr(p, '&');
  if (q != NULL) {
	  int l = min(outlen, q-p);
	  strncpy(outstr, p, q-p);
	  outstr[q-p] = '\0';
  } else {
	  strncpy(outstr, p, outlen);
	  outstr[outlen] = '\0';
  }
  return outstr;
}

static void parseSocketChangePayload(const char *payload, char **varname, int &varval, int &radio) {
  const int outlen = 40;
  static char outstr[outlen+1];
  char *tstr = getStrSocketPayloadParam(payload, "t", outstr, outlen);
  radio = tstr != NULL && strcmp(tstr, "radio") == 0;
  char *vstr = getStrSocketPayloadParam(payload, "v", outstr, outlen);
  varval = (strcmp(vstr, "true") == 0) ? 1 : (strcmp(vstr, "false") == 0) ? 0 : atoi(vstr);
  (*varname) = getStrSocketPayloadParam(payload, "n", outstr, outlen);
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
		else setVarByName(varname, varval, radio);
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
