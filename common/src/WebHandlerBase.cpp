/*
 * webhandler.cpp
 *
 *  Created on: Sep. 23, 2019
 *      Author: Danny
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

#include "FileSys.h"
#include "NTPtime.h"
#include "wifi.h"
#include "DelayTimings.h"
#include "NetSettings.h"
#include "DataSaver.h"

#include "WebHandlerBase.h"

#include "SerialDebugHelper.h"
#include "WebSocketBase.h"


#define MSG(x) SPRTLN(1, x)



static String getContentType(String filename) { // determine the filetype of a given filename, based on the extension
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

// borrowed from fancy-lights example code

bool WebHandlerBase::handleFileRead(String path) { // send the right file to the client (if it exists)
  //SPRTLN(1, "handleFileRead: " + path);
  if (! path.startsWith("/")) path = "/" + path;
  path = path.substring(path.lastIndexOf("/"));			   // Strip the directory as our filesystem is flat
  if (path.endsWith("/")) path = path + "index.html";      // If a folder is requested, send the index file
  String contentType = getContentType(path);               // Get the MIME type
  String pathWithGz = path + ".gz";
  if (FileSys.exists(pathWithGz) || FileSys.exists(path)) {  // If the file exists, either as a compressed archive, or normal
    if (FileSys.exists(pathWithGz))                        // If there's a compressed version available
      path += ".gz";                                       // Use the compressed verion
    File file = FileSys.open(path, "r");                   // Open the file
    httpServer.sendHeader("Cache-control", "max-age=2592000");
    size_t sent = httpServer.streamFile(file, contentType);      // Send it to the client
    file.close();                                          // Close the file again
    //SPRTLN(1, String("Sent file: ") + path);
    return true;
  }
  //SPRTLN(1, String("File Not Found: ") + path);     // If the file doesn't exist, return false
  return false;
}



#define CP(s) client.print(s)
#define CPN(s) client.println(s)
#define CPF(s) client.print(F(s))
#define CPNF(s) client.println(F(s))


// publish a message if the url doesn't correspond to anything
// i.e. send 404 response

void WebHandlerBase::handleNotFound(String path) {
	WiFiClient client = httpServer.client();
	CPNF("HTTP/1.1 404 NotFound");
	CPNF("Content-Type: text/plain");
	CPNF("Connection: close");
	CPN();
	String message = (httpServer.method() == HTTP_GET)?"GET":"POST";
	message += " ["+String(httpServer.args())+"] ";
	message += "  "+path+" ?";
	for (uint8_t i=0; i<httpServer.args(); i++){
		message += " " + httpServer.argName(i) + " = " + httpServer.arg(i);
	}
	message += "\n";
	CP(message);
}

void WebHandlerBase::handleIgnore() {
}

// handles serving files from file system & also path not found
// publishes the settings page if called in AP mode

void WebHandlerBase::handleGeneric() {
	String path = httpServer.uri();
	if (handleFileRead(path)) return;
	SPRNTF(1, "handle_generic: %s\r\n", path.c_str());
	if (WiFi.getMode() == WIFI_AP) {
		sendSettingsPage();
	} else {
		handleNotFound(path);
	}
}


// Debug page handler

static long InitialFreeHeapSize = 0;

static char debug_page_header[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<html lang="">
<head>
  <meta charset="UTF-8">
  <title>%s Debug</title>
</head>
<body>
<h2>%s Debug</h2>
)=====";

static char debug_page_form1[] PROGMEM = R"=====(
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
<a style="font-variant: small-caps;" href="/settings" title="Click to change the connection settings, e.g. if you are about to switch to a different network">settings</a>    
</h5>
<hr/>
<form method="POST" action="dbgputfile" enctype="multipart/form-data">
  <b>Files:</b>&nbsp;&nbsp;&nbsp;
  <input type="file" name="file" value="index.html" />
  <input class="button" type="submit" value="Upload" />
</form>
<p/>
<form method=GET action="dbggetfile">
  <select id="fileselect" name="file" size="3">
)=====";

static char debug_page_form2[] PROGMEM = R"=====(
  </select>
  <input type=submit name="subaction" value="Download" />
  <input type=submit name="subaction" value="Delete" />
</form>
<hr/>
<form method=GET action="dbgsetloopdelay">
Recent loop times: 
)=====";

static char debug_page_form3[] PROGMEM = R"=====(
  <input class="button" type="submit" name="subaction" value="Set" />
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
  <input class="button" type="submit" name="subaction" value="Restart" />
</form>
<hr/>
Wifi performance monitoring<br/>
)=====";

static char debug_page_trailer[] PROGMEM = R"=====(
</body>
</html>
)=====";


void WebHandlerBase::renderCommonDebugInfo() {
  WiFiClient client = httpServer.client();
  CPNF("HTTP/1.1 200 OK");
  CPNF("Content-Type: text/html");
  CPNF("Connection: close");
  CPN();
  MSG("sending debug page");
  client.printf_P(debug_page_header, textname, textname);

  CPF("<h4>");
  long uptime = NTPtimeManager.getUptime();
  if (uptime == 0) {
    CPF("Up for: "); CP(millis());
  } else {
	char tmbuff[20];
    CPF("Up since: "); CP(NTPtimeManager.getTimeString(20, tmbuff, uptime)); CPF(" UTC ["); CP(millis()); CPF(" ms]");
  }
  CPF(",&nbsp;&nbsp;   FreeHeap: "); CP(ESP.getFreeHeap()); CPF("/"); CP(InitialFreeHeapSize); CPNF("</h4>");
  CPF("<h5>");
  CPF("Connected as: &nbsp;&nbsp;"); CP(WiFi.hostname().c_str());
  CPF("&nbsp;&nbsp; to: &nbsp;&nbsp;"); CP(WiFi.SSID()); CPF("&nbsp;&nbsp;at: &nbsp;&nbsp;"); CP(WiFi.localIP());
  CPF(",&nbsp;&nbsp;&nbsp;&nbsp;   signal strength: "); CP(WiFi.RSSI());
  //CPNF("</h5>");   - see debug_page_form1

  CP(FPSTR(debug_page_form1));
  Dir dir = FileSys.openDir("/");
  while (dir.next()) {                      // List the file system contents
    String fileName = dir.fileName();
    size_t fileSize = dir.fileSize();
    client.printf("  <option value=\"%s\">%s,  (%d bytes)</option>\r\n", fileName.c_str(), fileName.c_str(), fileSize);
  }
  CP(FPSTR(debug_page_form2));
  float sum = 0, sumsq = 0, mean = 0, devn = 1; int n = 0, minn = 0, maxn = 0;
  for (;;) {
	  int v = get_timingdatum(n);
	  if (v <= -100) break;
	  client.printf("%s%d", n>0 ? ", ":"", v);
	  n += 1;
	  sum += v;
	  sumsq += v*v;
  }
  client.printf("\r\n<br/>\r\n");
  mean = sum/n;
  client.printf("N: %d, mean: %0.2f, deviation: %0.2f\r\n", n, mean, sqrt((sumsq-mean*sum)/(n-1)));
  client.printf("<br/>Delay time: \r\n");
  client.printf("<input type=\"text\" name=\"dt\" size=\"4\" value=\"%d\">\r\n", get_delay_time());
  CP(FPSTR(debug_page_form3));
  client.printf("<table>\r\n");
  client.printf("<tr><td>Connects: %d</td><td>Fails: %d</td><td>Fallbacks: %d</td></tr>\r\n",
		  wifi_instance->getStaConnectCount(), wifi_instance->getStaFailCount(), wifi_instance->getApFallbackCount());
  client.printf("<tr><td>Last success time: %0.3f seconds</td><td>Last failure time: %0.3f seconds</td></tr>\r\n",
		  wifi_instance->getLastStaSuccess()/1000.0, wifi_instance->getLastStaFailure()/1000.0);
  client.printf("<tr><td>Total station uptime: %0.3f</td><td>Total access point uptime: %0.3f</td></tr>\r\n",
		  wifi_instance->getTotalStaUptime()/1000.0, wifi_instance->getTotalApUptime()/1000.0);
  client.printf("</tr></table>");
}

void WebHandlerBase::handleDebugPage() {
	WiFiClient client = httpServer.client();
	renderCommonDebugInfo();
	renderAppDebugInfo();
	CP(FPSTR(debug_page_trailer));
}


void WebHandlerBase::debugSetLoopDelay() {
	if (! httpServer.hasArg("subaction")) return;			// neither button pushed??
	if (httpServer.arg("subaction").equals("Restart")) {
		{
			WiFiClient client = httpServer.client();
			CPNF("HTTP/1.1 200 OK");
			CPNF("Content-Type: text/html");
			CPNF("Connection: close");
			CPN();
			client.printf("restart pressed\r\n");
			client.flush();
		}
		delay(5000);								// need a handshake here!
		ESP.restart();
		return;										// never reached!
	}

	int dt;
	bool good = true;
	if (! httpServer.hasArg("dt")) good = false;
	else dt = atoi(httpServer.arg("dt").c_str());

	WiFiClient client = httpServer.client();
	CPNF("HTTP/1.1 200 OK");
	CPNF("Content-Type: text/html");
	CPNF("Connection: close");
	CPN();
	if (! good) {
		//SPRTLN(1, "failed to parse delay time for setting");
		client.println("failed setting loop delay time");
		return;
	}
	//SPRNTF(1, "setting loop delay time: dt: %d\r\n", dt);
	client.printf("setting loop delay time: dt: %d\r\n", dt);
	set_delay_time(dt);
	saveLibraryData();
}

// also: delete file
void WebHandlerBase::debugGetFile() {
	String fname, action;
	for (uint8_t i=0; i<httpServer.args(); i++) {
		if (httpServer.argName(i).equals("file")) fname = httpServer.arg(i);
		if (httpServer.argName(i).equals("subaction")) action = httpServer.arg(i);
	}
	MSG("debug_get_file: " + fname + ", action: " + action);
	if (action.equals("Download")) {
		if (FileSys.exists(fname)) {
			File file = FileSys.open(fname, "r");
			// specify the name to download by for the browser
			String hval = "attachment; filename="+fname.substring(1);	// skip leading '/'
			httpServer.sendHeader("Content-Disposition", hval.c_str());
			size_t sent = httpServer.streamFile(file, "application/octet-stream"); //send it to the client raw
			file.close();
			return;
		}
	} else if (action.equals("Delete")) {
		if (FileSys.exists(fname)) {
			FileSys.remove(fname);
			httpServer.send(200, "text/plain", "file deleted");
		}
	} else {
		MSG("unknown action in debug_get_file");
		httpServer.send(404, "text/plain", "unknown action");
	}
	MSG("File not found in debug_get_file");
	httpServer.send(404, "text/plain", "file not found");
}

void WebHandlerBase::debugPutFile() {
	HTTPUpload& upload = httpServer.upload();
	static File fsUploadFile;
	//SPRNTF(1, "uploading file '%s', status %d\r\n", upload.filename.c_str(), upload.status);
	//MSG("upload status = "+upload.status);
	if (upload.status == UPLOAD_FILE_START) {
	    String filename = upload.filename;
	    if (! filename.startsWith("/")) filename = "/"+filename;
	    MSG("debug_put_file "+filename);
	    fsUploadFile = FileSys.open(filename, "w");            // Open the file for writing in file system (create if it doesn't exist)
	    filename = String();
	} else if (upload.status == UPLOAD_FILE_WRITE) {
		//SPRINT(1, ".");
	    if (fsUploadFile) {
	    	fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
	    }
	} else if (upload.status == UPLOAD_FILE_END) {
	    if (fsUploadFile) {                                    // If the file was successfully created
	    	fsUploadFile.close();                               // Close the file again
	    	MSG("\r\nhandleFileUpload END Size: "); MSG(upload.totalSize);
	    	httpServer.sendHeader("Location", "/debug");      // Redirect the client back to the debug page
	    	httpServer.send(303);
	    } else {
	    	httpServer.send(500, "text/plain", "500: couldn't create file");
	    }
	}
}

// End debug page handler ^^^

// Settings page handler  vvv

// contains %s specs for formatting
static char settings_page[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<html lang="">
<head>
  <meta charset="UTF-8">
  <title>%s Settings</title>
</head>
<body>
<h1>%s Settings</h1>
<table style="width:25em">
<form method=GET action="setsettings">
<tr><td colspan="2">Give your %s a name here (no spaces or hyphens please)
and enter the credentials for connecting to your local network.
Then click "submit", connect back to your network,
and look for the controller as "http://name.local/".<p></td></tr>
<tr><td style="text-align:right">Client name</td><td><input type=text length=30 name="cname" value="%s"></td></tr>
<tr><td style="text-align:right">Network name</td><td><input type=text length=30 name="nname" value="%s"></td></tr>
<tr><td style="text-align:right">Network password</td><td><input type=password length=30 name="npass" value="%s"></td></tr>
<tr><td style="text-align:center"><input type="submit" name="resetaction" value="Submit"></td>
<td style="text-align:right"><input type="submit" name="resetaction" value="Reset everything" onclick="return maybereset()"></td></tr>
</table>
</form>
<script>
function maybereset() {
	return confirm("Are you sure you want to reset everything to the defaults?");
}
</script>
</body>
</html>
)=====";

void WebHandlerBase::sendSettingsPage() {
	MSG("serving settings page");
	WiFiClient client = httpServer.client();
	//SPRNTF(1, "connect for '%s' from %s\r\n", HTTP.uri().c_str(), client.remoteIP().toString().c_str());
	const int maxcflen = MAXNAMELEN+1;
	char publicname[maxcflen], ssid[maxcflen], password[maxcflen];
	if (! net_settings->read(MAXNAMELEN, publicname, ssid, password)) {
		publicname[0] = ssid[0] = password[0] = '\0';
	}
	client.printf_P(settings_page, textname, textname, textname, publicname, ssid, password);
}

void WebHandlerBase::saveSettings() {
	String action;
	//HTTP.send(200, "text/plain", "About to save settings.");
	for (uint8_t i=0; i<httpServer.args(); i++) {
		if (httpServer.argName(i).equals("resetaction")) action = httpServer.arg(i);
	}
	if (action.equals("Submit")) {
		MSG("saving settings");
		String pubname = httpServer.arg("cname");
		String netname = httpServer.arg("nname");
		String netpass = httpServer.arg("npass");
		if (pubname.length() >= MAXNAMELEN || netname.length() >= MAXNAMELEN || netpass.length() >= MAXNAMELEN) {
			MSG("settings parameter too long");
			httpServer.send(400, "text/plain", "Bad request denied.");
			return;
		}
		WiFiClient client = httpServer.client();
		if (net_settings->save(pubname.c_str(), netname.c_str(), netpass.c_str())) {
			MSG("saved settings");
			httpServer.send(200, "text/plain", "Settings saved.");
			wifi_settings_changed();
		} else {
			MSG("save of settings failed (string too long? file system error?");
			httpServer.send(200, "text/plain", "Settings rejected; try again.");
		}
	} else if (action.equals("Reset everything")) {
		MSG("resetting everything");
		net_settings->remove();
		webSocketHandler->deleteDataFromFile();
		httpServer.send(200, "text/plain", "Net settings have been deleted");	// (except for the bkp settings file)
		delay(1000);
		wifi_settings_changed();
	} else {
		MSG("bad request from settings page");
		httpServer.send(404, "text/plain", "Request not understood.");
	}
}


// note: the version of the server in the eclipse cdt does not delete its handlers
// on close. in fact there may be no way to delete the handlers.
// instead, we serve different pages according to mode

void WebHandlerBase::registerCommonRoutes(bool as_station) {
	static bool http_is_running = false;
	if (! http_is_running) {
		InitialFreeHeapSize = ESP.getFreeHeap();	// for future debugging
		http_is_running = true;
		//SPRNTF(1, "Starting HTTP...\r\n");
		httpServer.on("/setsettings", [this]() { this->saveSettings(); });
		httpServer.on("/debug", HTTP_GET, [this]() { this->handleDebugPage(); });
		httpServer.on("/dbgsetloopdelay", HTTP_GET, [this]() { this->debugSetLoopDelay(); });
		httpServer.on("/dbggetfile", HTTP_GET, [this]() { this->debugGetFile(); });
		httpServer.on("/dbgputfile",
				HTTP_POST,
				[this]() { httpServer.send(200, "text/plain", ""); },
				[this]() { this->debugPutFile(); });
		httpServer.on("/settings", HTTP_GET, [this]() { this->sendSettingsPage(); });
		//HTTP.on("/hotspot-detect.html", [this]() { this->handle_ignore(); });
		httpServer.on("/canonical.html", [this]() { this->handleIgnore(); });
		httpServer.on("/success.txt", [this]() { this->handleIgnore(); });
		httpServer.on("/bag", [this]() { this->handleIgnore(); });
		// conditional page serving, sends settings page in AP mode
		httpServer.onNotFound([this]() { this->handleGeneric(); });
		httpServer.on("/", HTTP_GET, [this]() { this->handleGeneric(); });
		httpServer.begin();
	}
	if (as_station) {
		MSG("Getting data from file system & starting web sockets");
		restoreLibraryData();							// tuning data
		restoreAppData();
		if (webSocketHandler) webSocketHandler->setupWebsocket();
	}
}

bool WebHandlerBase::saveLibraryData() {
	libraryData.dt = get_delay_time();
	return libraryDataSaver->write();
}

bool WebHandlerBase::restoreLibraryData() {
	bool r = libraryDataSaver->read();
	if (r) set_delay_time(libraryData.dt);
	return r;
}

const char *WebHandlerBase::textname = "";
const wifi *WebHandlerBase::wifi_instance = 0;

WebHandlerBase::WebHandlerBase(const char *txtnm) : webSocketHandler(0) {
	textname = txtnm;
	libraryDataSaver = new DataSaver(&libraryData, sizeof(libraryData), "/libconfig");
}

void WebHandlerBase::setWifi(wifi *thewifi) {
	wifi_instance = thewifi;			// for statistics reporting
}

void WebHandlerBase::setSocketHandler(WebSocketBase *sockethandler) {
	webSocketHandler = sockethandler;
}

void WebHandlerBase::setupWebhandling(bool as_station) {
	if (as_station) NTPtimeManager.begin();
	registerCommonRoutes(as_station);
	static bool http_is_running = false;
	if (! http_is_running) {
		http_is_running = true;
		registerAppRoutes();
	}
	DEBUG_WEBSOCKETS("websockets debugging is ON!");
}

void WebHandlerBase::loopWebhandling() {
	if (WiFi.getMode() == WIFI_STA) {
		NTPtimeManager.update();               // sync time every so often
		if (NTPtimeManager.isReady()) {
			static bool doneshowtime = false;
			if (! doneshowtime) {
				char tmbuff[20];
				SPRNTF(1, "%s\n", NTPtimeManager.getTimeString(20, tmbuff, NTPtimeManager.getAdjustedTime(0, 0, 0)));
				doneshowtime = true;
			}
		}
		if (webSocketHandler) webSocketHandler->loopWebsocket();
	}
	httpServer.handleClient();
}
