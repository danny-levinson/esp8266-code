/*
 * switch_controller.cpp
 *
 *  Created on: Nov. 3, 2026
 *      Author: Danny
 */


#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>

#include "switch_controller.h"
#include "MsgLogHandler.h"


String g_switchname = "Dishwasher fan";						// NEED TO MAKE THESE NON VOLATILE
int g_switchenabled = 1;


SwitchController::SwitchController() : msgcountin(0), msgcountout(0),
		switchIPAddress("") {

}

//========================== switch control (find & identify switch) ====================================

static String upnpmsg = "M-SEARCH * HTTP/1.1\r\nHOST:239.255.255.250:1900\r\nST:upnp:rootdevice\r\nMX:2\r\nMAN:\"ssdp:discover\"\r\n\r\n'";
static WiFiUDP multiscanudp;


void SwitchController::setupMulticastUDP() {
	MSGV("Starting Multicast UDP");
	multiscanudp.begin(65507);       // begin listening for a reply
}

void SwitchController::setupSwitch() {
	setupMulticastUDP();
}


// makes messages look odd in serial log, but fixes them in debug page log

static String escapeString(String s) {
	s.replace("<", "&lt;");
	s.replace(">", "&gt;");
	return s;
}

String SwitchController::getDeviceFriendlyName(String &loc) {
	WiFiClient client;
	HTTPClient http;
	http.begin(client, loc);
	int code = http.GET();
	MSGV("call to get '%s' returned '%d'", loc.c_str(), code);
	String payload = http.getString();
	http.end();
	if (code != 200) {
		MSGW("payload from get call was '%s'", escapeString(payload).c_str());
		return String("bad code "+String(code));
	}
	MSGV("got payload, len is "+String(payload.length()));
	char *tgt = "<friendlyName>";
	int l = payload.indexOf(tgt);
	if (l < 0) return "-";
	l += strlen(tgt);
	int m = payload.indexOf("<", l);
	return payload.substring(l, m);
}

void SwitchController::processIncomingPacket(String &incomingPacket, String &loc) {
	String s(incomingPacket);
	//MSGV("packet: %s", s.c_str());
	if (s.indexOf("uuid:Socket") >= 0 && s.indexOf("redsonic") >= 0) {      // seems to characterize wemo switches
		MSGV("got wemo, checking name");
		String name = getDeviceFriendlyName(loc);
		MSGV("name at loc '%s' is '%s', looking for '%s'", loc.c_str(), name.c_str(), g_switchname.c_str());
		if (g_switchname.length() != 0 && name.compareTo(g_switchname) == 0) {
			switchIPAddress = loc;
			MSGV("found switch '%s' at address '%s'", name.c_str(), loc.c_str());
		}
	}
}

static String getPacketLocation(String &s) {
	int l = s.indexOf("LOCATION:");
	if (l < 0) {
		return String("xxx");
	}
	int m = l + strlen("LOCATION:");
	while (s.charAt(m) == ' ') m++;
	int n = s.indexOf("\r", m);             // could probably use s.trim() to simplify a lot of this
	int p = s.indexOf("\n", m);
	int q;
	if (n < 0 && p < 0) return s.substring(m);
	if (n < 0 || p < 0) q = max(n, p);
	else q = min(n, p);
	return s.substring(m, q);
}

bool SwitchController::checkForMultiscanUdpMessage() {
	const int pbsz = 200;
	char pktbuf[pbsz] = "";
	int pktsize = multiscanudp.parsePacket();
	if (pktsize > 0) {
		msgcountin += 1;
		MSGV("got MUDP packet %d", msgcountin);
		if (switchIPAddress == "") {                                  // if we already have the switch, just flush messages
			//MSGV("MUDP received %d bytes from %s, port %d", pktsize, multiscanudp.remoteIP().toString().c_str(), multiscanudp.remotePort());
			// include/UdpContext.h documents that this can be inaccurate if user code doesn't swallow packets fast enough!
			//String rip = multiscanudp.remoteIP().toString();
			String pkt("");
			while (multiscanudp.available() > 0) {
				int len = multiscanudp.read(pktbuf, pbsz-1);    // leave room for terminator
				if (len > 0) pktbuf[len] = 0;                   // maybe it already does this?
				pkt.concat(pktbuf);
			}
			String loc = getPacketLocation(pkt);
			MSGV("MUDP LOC: " + loc);
			processIncomingPacket(pkt, loc);
		}
		multiscanudp.flush();
		return true;
	}
	else {
		//MSGV("no packet to parse");
		return false;
	}
}

// called from application loop

void SwitchController::checkForMultiscanUdpMessages() {
	for (int i=0; i<20; i++) {
		if (! checkForMultiscanUdpMessage()) return;
	}
}

// currently: ask for responses every 60 secs till found, then check every 10 minutes
// called from webhandler loop

void SwitchController::perhapsAskForSwitchAddress() {
	if (! g_switchenabled) return;
	if (g_switchname.length() == 0) return;
	static unsigned long intervalSWAsk = 60000;               		// Request switch info every 60k msecs = 60 seconds
	static unsigned long intervalSWCheck = 1200000;      			// interval after first finding switch is 1200k=20 minutes
	static unsigned long prevSWQuery = 0;
	unsigned long currentMillis = millis();
	if (switchIPAddress != "") {                               		// just check, don't multicast
		if (currentMillis - prevSWQuery > intervalSWCheck) {    	// If desired interval has passed since last switch request
			prevSWQuery = currentMillis;
			MSGV("check switch response");
			String name = getDeviceFriendlyName(switchIPAddress);
			if (name.compareTo(g_switchname) != 0) {
				switchIPAddress = "";
				MSGV("casting for switch again");					// cast next time round
			}
		}
	} else {
		if (prevSWQuery == 0 || currentMillis - prevSWQuery > intervalSWAsk) { // If desired interval has passed since last SW switch request
			prevSWQuery = currentMillis;
			MSGV("casting for switch (msg: %d)", msgcountout+1);
			multiscanudp.beginPacketMulticast({239, 255, 255, 250}, 1900, WiFi.localIP());    // 1900 is ssdp port
			multiscanudp.write(upnpmsg.c_str());
			multiscanudp.endPacket();
			msgcountout += 1;
		}
	}
}

void SwitchController::loopSwitch() {
	if (g_switchenabled) {
	    perhapsAskForSwitchAddress();
	    checkForMultiscanUdpMessages();					// moved from templogger loop
	  }
}
