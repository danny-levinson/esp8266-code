/*
 * WebSocketBase.cpp
 *
 *  Created on: Aug. 20, 2026
 *      Author: Danny
 */

#include <ArduinoJson.h>
#include <WebSocketsServer.h>

#include "NTPtime.h"
#include "WebSocketBase.h"
#include "CommonGlobals.h"

#include "SerialDebugHelper.h"


#define MSG(x) SPRTLN(1, x)


WebSocketBase::WebSocketBase(uint16_t port)
    : webSocket(port) // @suppress("Symbol is not resolved")
{
}

void WebSocketBase::setupWebsocket()
{
	common_globals.restoreDataFromFile();					// state data
    webSocket.begin();
    webSocket.onEvent([this](uint8_t num, WStype_t type, uint8_t *payload, size_t len) {
        String msg;

        if (type == WStype_TEXT) {
            msg = String((char*)payload).substring(0, len);
        }

        // Common handling first
        if (! this->handleCommonWebSocketMessage(num, type, msg)) {

        	// App-specific handling if not handled by common
        	this->handleAppWebSocketMessage(num, type, msg);
        }
    });
}

void WebSocketBase::loopWebsocket()
{
	webSocket.loop();
}

// special "times":
//	999 - initial value in web page
//  888 - websocket is open, interval timer is set up
//  777 - ntp server has not yet responded

void WebSocketBase::sendTimeData(uint8_t clientnum) {
	int tzone, dst, offset;
	SPRNTF(1, "Sending time data (sckt): %d, %d, %d\n", cg_tzone, cg_dst, cg_offset);
	StaticJsonDocument<100> jsonBuffer;     // .add fails silently if space is insufficient
	JsonObject root = jsonBuffer.createNestedObject();
	//root["seqNum"] = seqnum;
	root["curTime"] = "777";
	if (NTPtimeManager.isReady()) {
		static char tmbuff[20];
		root["curTime"] = NTPtimeManager.getTimeString(20, tmbuff, NTPtimeManager.getAdjustedTime(tzone, dst, offset));
		SPRNTF(1, "curTime = %s (from %s)\n", root["curTime"], tmbuff);
	}
	//SPRNTF(1, "jsonBuffer usage: %d\r\n", jsonBuffer.memoryUsage()); // last measured at 50
	String jsonout;
	serializeJson(root, jsonout);
	//MSG("sent bytes: "+String(jsonout.length())); // last measured at 31
	wsSend(clientnum, jsonout.c_str(), jsonout.length());
}

bool WebSocketBase::handleCommonWebSocketMessage(uint8_t num,
                                                 WStype_t type,
                                                 const String &msg)
{
	String pfx = "["+String(num)+"] ";		// num is the websocket connection id
	switch (type) {
	default:
		MSG(pfx+"type: "+String(type));
		return false;

	case WStype_CONNECTED: {               // if a new websocket connection is established
        IPAddress ip = webSocket.remoteIP(num);
        MSG(pfx+"Connected from "+String(ip[0])+"."+String(ip[1])+"."+String(ip[2])+"."+String(ip[3])+" url: "+msg);
    	}
		wsSend(num, "hello from base");
    	return true;

	case WStype_TEXT:
        if (msg == "ping") {
        	wsSend(num, "pong");
        	return true;
        }
        if (strstr(msg.c_str(), "/updatetime") != NULL) {
        	sendTimeData(num);
        	return true;
        }
        return false;

    case WStype_DISCONNECTED:             // if the websocket is disconnected
    	MSG(pfx+"Disconnected!\n");
    	return true;
    }
	return false;
}

void WebSocketBase::wsSend(uint8_t num, const String &msg)
{
	webSocket.sendTXT(num, msg.c_str());
}


void WebSocketBase::wsSend(uint8_t num, const char *msg, int len)
{
	webSocket.sendTXT(num, msg, len);
}


char *WebSocketBase::getStrSocketPayloadParam(const char *payload, char *param, char *outstr, int outlen) {
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

void WebSocketBase::parseSocketChangePayload(const char *payload, char **varname, int &varval, int &radio) {
  const int outlen = 40;
  static char outstr[outlen+1];
  char *tstr = getStrSocketPayloadParam(payload, "t", outstr, outlen);
  radio = tstr != NULL && strcmp(tstr, "radio") == 0;
  char *vstr = getStrSocketPayloadParam(payload, "v", outstr, outlen);
  varval = (strcmp(vstr, "true") == 0) ? 1 : (strcmp(vstr, "false") == 0) ? 0 : atoi(vstr);
  (*varname) = getStrSocketPayloadParam(payload, "n", outstr, outlen);
}
