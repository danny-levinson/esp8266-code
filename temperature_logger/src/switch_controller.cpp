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

#include "WebHandlerBase.h"
#include "DataSaver.h"
#include "switch_controller.h"
#include "temperature_logger.h"
#include "MsgLogHandler.h"

char g_switchname[MAXSWITCHNAMELEN] = "Dishwasher fanX";						// NEED TO MAKE THESE NON VOLATILE
int g_switchenabled = 1, g_switchauto = 0;
int g_switchon = 20, g_switchoff = 60;						// in tenths of a degree

static const char *switchConfigDataFilename = "/switchconfig.dat";

SwitchController::SwitchController(class WebHandlerBase *wbh) : msgcountin(0), msgcountout(0), webhandler(wbh),
		switchIPAddress("") {
	switchConfigDataSaver = new DataSaver(&switchConfigData, sizeof(switchConfigData), switchConfigDataFilename);
}

bool SwitchController::saveSwitchData() {
	MSGV("Saving switch data: enabled = %d, auto = %d, on = %d, off = %d, name = '%s'",
			g_switchenabled, g_switchauto, g_switchon, g_switchoff, g_switchname);
	switchConfigData.switchauto = g_switchauto;
	switchConfigData.switchenabled = g_switchenabled;
	switchConfigData.switchon = g_switchon;
	switchConfigData.switchoff = g_switchoff;
	strcpy(switchConfigData.switchname, g_switchname);	// safe as long as g_switchname gets checked
	return switchConfigDataSaver->write();
}

bool SwitchController::restoreSwitchData() {
	if (! switchConfigDataSaver->read()) {
		MSGV("switchConfigDataSaver read failed");
		return false;
	}
	g_switchauto = switchConfigData.switchauto;
	g_switchenabled = switchConfigData.switchenabled;
	g_switchon = switchConfigData.switchon;
	g_switchoff = switchConfigData.switchoff;
	strcpy(g_switchname, switchConfigData.switchname);	// safe if config file init to zero & max size not changed
	MSGV("Switch data restored: enabled=%d, auto=%d, on=%d, off=%d, name='%s'",
			g_switchenabled, g_switchauto, g_switchon, g_switchoff, g_switchname);
	return true;
}

// the only routes to register for the switch controller are for the configuration buttons

void SwitchController::registerSwitchRoutes(ESP8266WebServer &httpServer) {
	httpServer.on("/setswitchparameters", HTTP_GET, [this]() { this->setSwitchParameters(); });
	httpServer.on("/resetswitchparameters", HTTP_GET, [this]() { this->resetSwitchParameters(); });
	httpServer.on("/toggleswenable", HTTP_GET, [this]() { this->toggleSwitchEnable(); });
	httpServer.on("/toggleswauto", HTTP_GET, [this]() { this->toggleSwitchAutomatic(); });
}

bool SwitchController::setSwitchParametersFromServer() {
	ESP8266WebServer &httpServer = webhandler->getServer();
	MSGV("switch params from httpserver: hasname=%d, name=%s", httpServer.hasArg("swname"), httpServer.hasArg("swname") ? httpServer.arg("swname").c_str() : "XX");
	bool good = true;
	// if switch is disabled these params will not be sent, so use old values
	if (httpServer.hasArg("thon")) g_switchon = int(10 * atof(httpServer.arg("thon").c_str()));
	if (httpServer.hasArg("thoff")) g_switchoff = int (10 * atof(httpServer.arg("thoff").c_str()));
	if (httpServer.hasArg("swname")) strncpy(g_switchname, httpServer.arg("swname").c_str(), MAXSWITCHNAMELEN);
	g_switchenabled = httpServer.hasArg("swenable") ? 1 : 0;
	g_switchauto = httpServer.hasArg("swauto") ? 1 : 0;
	if (good) good = saveSwitchData();
	return good;
}

void SwitchController::setSwitchParameters() {
	WiFiClient &client = webhandler->sendHeaderToClient();
	MSGV("sending configuration switch info after setting");
	bool good = setSwitchParametersFromServer();				// also saves common globals?
	if (good) {
		MSGV("*set* switch parameters set successfully");
	} else {
		MSGE("*set* failed setting switch parameters");
	}
	configSendSwitchSection(client, good ? "setting parameters successful" : "setting parameters failed");
}

void SwitchController::resetSwitchParameters() {
	WiFiClient &client = webhandler->sendHeaderToClient();
	MSGV("*reset* cancelled changing & reverted configuration switch info data");
	configSendSwitchSection(client, "parameters reset");
	//sendTimeInfo(client, timeInfoSection, "parameters reset");
}


// handles actions in the switch section of the config form:
//		changes in parameters (on temperature, off temperature, device name)
//		checkbox changes (enable, automatic)
//		button pushes (save data, test switch)
static const char *save_button_text = "Save switch configuration changes";

static const char switch_section_format[] PROGMEM = R"=====(
<style scoped="">#swtoast.htmx-settling { opacity: 100; } #swtoast { background: #E1F0DA; opacity: 0; transition: opacity 3s ease-out; }</style>
<span id="switchdata">
<form style="display: inline;" hx-get="/setswitchparameters" hx-target="#switchdata" hx-swap="innerHTML settle:1s">
<label for="swname" title="Name given to the switch, as known to the switching device">Name:&nbsp;
<input type="text" id="swname" name="swname" value="%s" %s length="20"></label>
&nbsp;&nbsp;&nbsp;&nbsp;
<button class="btn" type="submit" title="Save changes to the switch settings (name & thresholds).">%s</button>
<button class="btn" hx-get="/resetswitchparameters" title="Revert the switch settings">Cancel</button>
<br/>
<label for="thon" title="Threshold for switching the device on if temperature read is lower">On threshold
<input type="number" id="thon" name="thon" value="%0.1f" step="0.1" size="6" %s></label>
&nbsp;&nbsp;&nbsp;
<label for="thoff" title="Threshold for switching the device off if temperature read is higher">Off threshold
<input type="number" id="thoff" name="thoff" value="%0.1f" step="0.1" size="6" %s></label>
&nbsp;&nbsp;&nbsp;&nbsp;
<label for="swenable" title="Configure an external switch (such as a WEMO, connected, for example, to a heater) to operate according to temperature limits. If switch control is disabled, the logger will not attempt to find a switch.">
Control is:&nbsp;&nbsp; enabled
<input type="checkbox" id="swenable" name="swenable" %s hx-get="/toggleswenable" hx-target="#switchdata" hx-swap="innerHTML settle:1s"></label>
&nbsp;&nbsp;&nbsp;
<label for="swauto" title="Make the switch respond to the set temperature limits. Switch control can be set to manual for testing. In that case, the switch, when found, can be controlled by a button below.">
automatic
<input type="checkbox" id="swauto" name="swauto" %s></label>
</form>
<br/>
%s
&nbsp;&nbsp;&nbsp;&nbsp;
)=====";

static char switch_section_part2[] PROGMEM = R"=====(
&nbsp;&nbsp;&nbsp;
<input class="button" id="swswitch" type="submit" title="Turn the device on or off for testing purposes" name="subaction" value="Switch device %s" />
&nbsp;&nbsp;&nbsp;&nbsp;
</span>
<span id="swtoast" hx-swap-oob="true"> %s </span>
)=====";

//<input type="checkbox" id="swenable" name="swenable" %s onclick="enableDisable(this.checked);"></label>
//<input type="checkbox" id="swauto" name="swauto" %s onclick="showButton(this.checked);"></label>

#if 0
static const char switch_section_script[] PROGMEM = R"=====(
<script>
function enableDisable(bEnable) {
	document.getElementById("swname").disabled = !bEnable;
	document.getElementById("thon").disabled = !bEnable;
	document.getElementById("thoff").disabled = !bEnable;
}
function showButton(bHideit) {
  var x = document.getElementById("swswitch");
  x.style.visibility = bHideit ? 'hidden' : 'visible';
}
</script>
)=====";

static const char switch_section_actions[] PROGMEM = R"=====(
<script>
enableDisable(document.getElementById("swenable").checked);
showButton(document.getElementById("swauto").checked);
</script>
)=====";
#endif

void SwitchController::configSendSwitchSection(WiFiClient &client, const char *toast) {
	MSGV("sending switch configuration section");
	String swaddr = switchAddress();
	//client.printf_P(switch_section_script);
	const char *enablestate = g_switchenabled ? "enabled" : "disabled";
	client.printf_P(switch_section_format,
			g_switchname, enablestate,
			save_button_text,
			g_switchon/10.0, enablestate,
			g_switchoff/10.0, enablestate,
			g_switchenabled ? "checked" : "xxx",
			g_switchauto ? "checked" : "yyy",
			swaddr != "" ? "Switch is online.&nbsp;" : "Switch not (yet) found");
	MSGV("config has enable: %d, auto: %d, on: %d, off: %d, name: '%s'",
			g_switchenabled, g_switchauto, g_switchon, g_switchoff, g_switchname);

	int r = getCurrentSwitchState();
	if (swaddr != "") {
		//char *spaces = "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;";
		client.printf("'<i>%s</i>'&nbsp; is at %s.<br/>", g_switchname, swaddr.c_str());
		client.printf("Switch state is %s.&nbsp;&nbsp;", (r == 0) ? "off" : (r == 1) ? "on" : "unknown");
	}
	client.printf_P(switch_section_part2, r == 0 ? "on" : r == 1 ? "off" : "is undefined", toast);

	//client.printf_P(switch_section_actions);
}

void SwitchController::toggleSwitchEnable() {
	WiFiClient &client = webhandler->sendHeaderToClient();
	MSGV("sending configuration switch info after setting");
	g_switchenabled = ! g_switchenabled;
	bool good = saveSwitchData();
	configSendSwitchSection(client, g_switchenabled ? "enabled" : "disabled");
}

void SwitchController::toggleSwitchAutomatic() {

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
		MSGV("name at loc '%s' is '%s', looking for '%s'", loc.c_str(), name.c_str(), g_switchname);
		if (strlen(g_switchname) != 0 && strcmp(name.c_str(), g_switchname) == 0) {
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
	if (strlen(g_switchname) == 0) return;
	if (! webhandler->wifiIsConnected()) return;
	static unsigned long intervalSWAsk = 60000;               		// Request switch info every 60k msecs = 60 seconds
	static unsigned long intervalSWCheck = 1200000;      			// interval after first finding switch is 1200k=20 minutes
	static unsigned long prevSWQuery = 0;
	unsigned long currentMillis = millis();
	if (switchIPAddress != "") {                               		// just check, don't multicast
		if (currentMillis - prevSWQuery > intervalSWCheck) {    	// If desired interval has passed since last switch request
			prevSWQuery = currentMillis;
			MSGV("check switch response");
			String name = getDeviceFriendlyName(switchIPAddress);
			if (strcmp(name.c_str(), g_switchname) != 0) {
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


//========================== switch control (actuate switch) ====================================

// *** should become class variables...

static int switchState = -1;								// -1 unknown/unconfirmed, 0 off, 1 on

static bool desiredSwitchState = false;						// intended state, may not be confirmed

static unsigned long lastSwitchStateCheckTime = 0;
static unsigned long switchStateCheckInterval = 10000;      // 10 seconds

static char *wemo_set_op_1 ="<?xml version=\"1.0\" encoding=\"utf-8\"?><s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body><u:SetBinaryState xmlns:u=\"urn:Belkin:service:basicevent:1\"><BinaryState>";
static char *wemo_set_op_2 = "</BinaryState></u:SetBinaryState></s:Body></s:Envelope>";
static char *wemo_get_op_1 ="<?xml version=\"1.0\" encoding=\"utf-8\"?><s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body><u:GetBinaryState xmlns:u=\"urn:Belkin:service:basicevent:1\"><BinaryState>";
static char *wemo_get_op_2 = "</BinaryState></u:GetBinaryState></s:Body></s:Envelope>";

static const int httpclienttimeout = 20000;


// *** change doc to config page refs when implemented

// called from debug_send_page in webdebug.cpp to test for & show switch address
// called from config_send_page in webhandler.cpp to show switch status (online vs not found)
// also called in checkSwitchState()

String SwitchController::switchAddress() {
	return switchIPAddress;
}

// soap handling routines
// returns: -1 unknown, 0 off, 1 on

int SwitchController::getSwitchState() {
	if (switchIPAddress == "") return -1;
	MSGV("Getting state from switch");
	static String cmd = wemo_get_op_1;
	cmd.concat("1");              // probably doesn't matter
	cmd.concat(wemo_get_op_2);
	String addr = switchIPAddress.substring(0, switchIPAddress.length()-9);  // cut the trailing 'setup.xml'
	addr.concat("upnp/control/basicevent1");
	MSGV("address: [%d] %s", addr.length(), addr.c_str());
	//MSGV("command: [%d] %s", cmd.length(), cmd.substring(0,60).c_str());	// can't log cmd without escaping xml brackets!
	WiFiClient client;
	HTTPClient http;
	http.begin(client, addr);
	MSGV("begun client, timeout was: %d", client.getTimeout());
	client.setTimeout(httpclienttimeout);					// leave tcp timeout at 5000 but up client timeout, may help with soap call
	http.addHeader("Accept", "");
	http.addHeader("Content-type", "text/xml; charset=\"utf-8\"");
	http.addHeader("SOAPACTION", "\"urn:Belkin:service:basicevent:1#GetBinaryState\"");
	int code = http.POST(cmd);
	String payload = http.getString();
	http.end();
	if (payload.length() == 0) {
		MSGW("zero length payload from switch");
		return -1;   // no reply?
	}
	char *statetag = "<BinaryState>";
	char *p = strstr(payload.c_str(), statetag);
	if (p == NULL) {
		MSGW("could not find state tag in payload (length %d) from switch", payload.length());
		MSGW("payload: %s", escapeString(payload).c_str());				// will screw up logging because of html in payload
		return -1;
	}
	char statech = *(p+strlen(statetag));
	bool on = false;
	if (statech == '1') on = true;
	else if (statech != '0') {
		MSGW("bad state character in switch payload");
		return -1;
	}
	MSGV("Switch state is " + String((on ? "on" : "off")));
	return on;   // return state
}

bool SwitchController::setSwitchState(bool onOrOff) {
	if (switchIPAddress == "") return false;
	MSGV("Switching device " + String((onOrOff ? "on" : "off")));
	String cmd = wemo_set_op_1;
	cmd.concat(onOrOff ? "1" : "0");
	cmd.concat(wemo_set_op_2);
	String addr = switchIPAddress.substring(0, switchIPAddress.length()-9);  // cut the trailing 'setup.xml'
	addr.concat("upnp/control/basicevent1");
	WiFiClient client;
	HTTPClient http;
	http.begin(client, addr);
	MSGV("begun client, timeout was: %d", client.getTimeout());
	client.setTimeout(httpclienttimeout);					// leave tcp timeout at 5000 but up client timeout, may help with soap call
	http.addHeader("Accept", "");
	http.addHeader("Content-type", "text/xml; charset=\"utf-8\"");
	http.addHeader("SOAPACTION", "\"urn:Belkin:service:basicevent:1#SetBinaryState\"");
	int code = http.POST(cmd);
	String payload = http.getString();
	MSGV("received from client: code %d, payload length: %d, addr: %s, cmd length: %d", code, payload.length(), addr.c_str(), cmd.length());
	http.end();
	desiredSwitchState = onOrOff;
	switchState = -1;					// state still formally unknown
	lastSwitchStateCheckTime = 0;		// force a check next time round
	return true;
}

// if switch state is unknown (-1), ask the switch for it

int SwitchController::checkSwitchState() {
	if (switchAddress() == "") return -1;
	if (switchState == -1) {
		switchState = getSwitchState();
	}
	return switchState;
}

void SwitchController::unsetSwitchState() {
	switchState = -1;
}

// just get the recorded switch state without asking the switch for it

int SwitchController::getCurrentSwitchState() {
	return switchState;			// could be -1
}

bool SwitchController::isSwitchOn() {
	return switchState == 1;
}


void SwitchController::maybeActuateSwitch() {
	//if (strcmp(location, "kitchen") != 0) return;           // make sure we are the right temperature monitor - no longer hardwired
	if (switchIPAddress == "") return;                        // can't switch it if we haven't got a switch

	float lastTemperatureReading = the_temperature_logger.getLastTemperature();
	if (lastTemperatureReading >= 9999.0) return;

	float switchOnTemperature  = g_switchon  / 10.0,
		  switchOffTemperature = g_switchoff / 10.0;  			// was originally 6 & 9, lately 2 & 6	-- now part of config

	if (desiredSwitchState && lastTemperatureReading >= switchOffTemperature) {			// on & want off
		setSwitchState(false);
	}
	else if (! desiredSwitchState && lastTemperatureReading <= switchOnTemperature) {	// off & want on
		setSwitchState(true);
	}

	unsigned long currentTime = millis();
	if (! switchState != -1 && currentTime - lastSwitchStateCheckTime > switchStateCheckInterval) {
		checkSwitchState();										// read back to make sure switch was set successfully
		lastSwitchStateCheckTime = currentTime;
	}

}

void SwitchController::loopSwitch() {
	if (g_switchenabled) {
	    perhapsAskForSwitchAddress();
	    checkForMultiscanUdpMessages();					// moved from templogger loop
	    maybeActuateSwitch();
	  }
}
