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

#include "SerialDebugHelper.h"


#define MSG(x) SPRTLN(1, x)


WebSocketBase::WebSocketBase(uint16_t port)
    : webSocket(port) // @suppress("Symbol is not resolved")
{
}

void WebSocketBase::setupWebsocket()
{
	restoreDataFromFile();					// state data
    webSocket.begin();
    webSocket.onEvent([this](uint8_t num, WStype_t type, uint8_t *payload, size_t len) {
        String msg;

        if (type == WStype_TEXT) {
            msg = String((char*)payload).substring(0, len);
        }

        // Common handling first
        this->handleCommonWebSocketMessage(num, type, msg);

        // App-specific handling
        this->handleAppWebSocketMessage(num, type, msg);
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
	getTimeAdjustmentParameters(tzone, dst, offset);
	SPRNTF(1, "Sending time data: %d, %d, %d\n", tzone, dst, offset);
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

void WebSocketBase::handleCommonWebSocketMessage(uint8_t num,
                                                 WStype_t type,
                                                 const String &msg)
{
	String pfx = "["+String(num)+"] ";		// num is the websocket connection id
	switch (type) {
	default:
		MSG(pfx+"type: "+String(type));
		break;

	case WStype_CONNECTED: {               // if a new websocket connection is established
        IPAddress ip = webSocket.remoteIP(num);
        MSG(pfx+"Connected from "+String(ip[0])+"."+String(ip[1])+"."+String(ip[2])+"."+String(ip[3])+" url: "+msg);
    	}
		wsSend(num, "hello from base");
    	break;

	case WStype_TEXT:
        if (msg == "ping") wsSend(num, "pong");
        if (strstr(msg.c_str(), "/updatetime") != NULL) sendTimeData(num);
        break;

    case WStype_DISCONNECTED:             // if the websocket is disconnected
    	MSG(pfx+"Disconnected!\n");
    	break;
    }
}

void WebSocketBase::wsSend(uint8_t num, const String &msg)
{
	webSocket.sendTXT(num, msg.c_str());
}


void WebSocketBase::wsSend(uint8_t num, const char *msg, int len)
{
	webSocket.sendTXT(num, msg, len);
}

