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
#include "damper.h"
#include "gdata.h"


static bool data_was_initialised = false;
static String datafilename = "/gdata";


void WebSocketApp::getTimeAdjustmentParameters(int &tz, int &dst, int &offst) {
	tz = g_tzone;
	dst = g_dst;
	offst = g_offset;
}

bool WebSocketApp::restoreDataFromFile() {
    File f = FileSys.open(datafilename+".dat", "r");
    if (! f) {
    	SPRTLN(1, "can't open .dat file for reading"); // @suppress("Method cannot be resolved")
    	data_was_initialised = false;
    	return false;
    }
    while (f.available() > 0) {
    	static int gdptr = 0;
    	String s = f.readStringUntil('\0');
    	int d = 0;
    	f.readBytes((char*)&d, sizeof(int));
    	//SPRNTF(1, "got '%s' = %d (%d)\r\n", s.c_str(), d, f.available());
		if (s.equals(g_data_with_names[gdptr].uiname)) {
			*g_data_with_names[gdptr++].gvar = d;		// this should usually work
		} else {
			// otherwise, it's linear search
			for (gdptr=0; g_data_with_names[gdptr].gvar != NULL; gdptr++) {
				if (s.equals(g_data_with_names[gdptr].uiname)) {
					*g_data_with_names[gdptr++].gvar = d;
					break;
				}
			}
		}
		if (g_data_with_names[gdptr].gvar == NULL) gdptr = 0;
    }
    data_was_initialised = true;
    SPRNTF(1, "restored data from file, offset %d, tzone: %d, dst: %d\n", g_offset, g_tzone, g_dst); // @suppress("Method cannot be resolved")
   	return true;
}

void WebSocketApp::saveDataToFile() {
    File f = FileSys.open(datafilename+".new", "w");
	for (int i=0; g_data_with_names[i].gvar != NULL; i++) {
		f.write((char*)g_data_with_names[i].uiname);
		f.write((char)'\0');		// write doesn't null terminate strings so we write a (char)0 [not (char*)]
		f.write((char*)(&*g_data_with_names[i].gvar), sizeof(int));
	}
	f.close();
	if (FileSys.exists(datafilename+".bkp")) FileSys.remove(datafilename+".bkp");
	if (FileSys.exists(datafilename+".dat")) FileSys.rename(datafilename+".dat", datafilename+".bkp");
	FileSys.rename(datafilename+".new", datafilename+".dat");
	data_was_initialised = true;
    SPRNTF(1, "saved data to file, offset %d, tzone: %d, dst: %d\n", g_offset, g_tzone, g_dst);
	SPRTLN(1, "data saved to file");
	restoreDataFromFile();
}

// definitions of global constants declared in gdata.h & set by ui; initialized from file system file

int g_offset, g_tzone, g_dst, g_manual, g_damperopen;


g_data_struct g_data_with_names[] = {
	&g_offset,       "offset",
	&g_tzone,        "tzone",
	&g_dst,          "dst",
	&g_manual,		 "manual",
	&g_damperopen,	 "damperopen",
	NULL, NULL
};

//extern int real_demolevel;


// we don't delete the .bkp file though
void WebSocketApp::deleteDataFromFile() {
	if (FileSys.exists(datafilename+".dat")) {
		FileSys.remove(datafilename+".dat");
	}
	data_was_initialised = false;
}

void WebSocketApp::showGVars() {
	char outstr[120] = "", *p = outstr;
	for (int i=0; g_data_with_names[i].gvar != NULL; i++) {
		sprintf(p, "%s=%d, ", g_data_with_names[i].uiname, *g_data_with_names[i].gvar);
		p += strlen(p);
		if (p - outstr > 50) {
			SPRTLN(1, outstr); p = outstr; outstr[0] = '\0';
		}
	}
	if (p != outstr) SPRTLN(1, outstr);
	saveDataToFile();
}

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
			saveDataToFile();
			return;
		}
	}
	SPRTLN(1, String("gvar matching '")+varname+"' not found");
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
	saveDataToFile();
}


void WebSocketApp::sendInitData(uint8_t clientnum) {
	StaticJsonDocument<1500> jsonBuffer;     // .add fails silently if space is insufficient
	JsonObject root = jsonBuffer.createNestedObject();
	JsonObject data = jsonBuffer.createNestedObject();
	int i;
	if (data_was_initialised) {
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
	saveDataToFile();
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
	saveDataToFile();
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
	saveDataToFile();
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


WebSocketApp::WebSocketApp(uint16_t port)
    : WebSocketBase(port)
{
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
		showGVars();
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
