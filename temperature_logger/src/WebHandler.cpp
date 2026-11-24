/*
 * WebHandler.cpp
 *
 *  Created on: Sep. 29, 2026
 *      Author: Danny
 */

#include <ESP8266SSDP.h>

#include "MsgLogHandler.h"
#include "FileSys.h"
#include "NTPtime.h"

#include "WebHandler.h"
#include "DataSaver.h"
#include "CommonGlobals.h"

#include "temperature_logger.h"
#include "temperature_filing.h"
#include "switch_controller.h"

static const char *appVersion = "1.0";	// represents app (non-common) part of version for entire app

static float g_calib = 0.0;

void WebHandler::registerAppRoutes(bool asStation) {
	httpServer.on("/index.html", HTTP_GET, [this]() { this->temperaturesSendPage(); });
	httpServer.on("/temperatures.html", HTTP_GET, [this]() { this->temperaturesSendPage(); });
	httpServer.on("/config.html", HTTP_GET, [this]() { this->configSendPage(); });
	httpServer.on("/config", HTTP_GET, [this]() { this->configSendPage(); });
	//httpServer.on("/configset", HTTP_GET, [this]() { this->configSaveData(); });
	httpServer.on("/changetemperatureoffset", HTTP_GET, [this]() { this->changeTemperatureOffset(); });
	httpServer.on("/settemperatureoffset", HTTP_GET, [this]() { this->setTemperatureOffset(); });
	httpServer.on("/resettemperatureoffset", HTTP_GET, [this]() { this->resetTemperatureOffset(); });
	httpServer.on("/settimeparameters", HTTP_GET, [this]() { this->setTimeParameters(); });
	httpServer.on("/resettimeparameters", HTTP_GET, [this]() { this->resetTimeParameters(); });
	httpServer.on("/updatemessages", HTTP_GET, [this]() { this->updateMessages(); });

	httpServer.on("/sync", HTTP_GET, [this]() { this->syncBuffer(); });
	httpServer.on("/listspiffstoclient", HTTP_GET, [this]() { this->listFilesToClient(); });	// obsolescent url
	httpServer.on("/listfilestoclient", HTTP_GET, [this]() { this->listFilesToClient(); });
	httpServer.on("/getrawfile", HTTP_GET, [this]() { this->getRawFile(); });
	httpServer.on("/getrawfilecrc32", HTTP_GET, [this]() { this->getRawFileChecksum(); });
	httpServer.on("/getfile", HTTP_GET, [this]() { this->getRawFile(); });			// obsolescent url (base uses /dbggetfile)

	httpServer.on("/testbutton", HTTP_GET, [this]() { this->getTestButton(); });
	the_switch_controller->registerSwitchRoutes(httpServer);

	if (asStation) setupSSDP();
}

void WebHandler::appLoopWebhandling() {
	sendNextFileChunk();				// does nothing unless file is being sent
}

const char *WebHandler::getAppVersion() {
	return appVersion;
}

static const char *appDataFilename = "/appconfig.dat";

WebHandler::WebHandler(const char *genericname) : WebHandlerBase(genericname),
		sendingFileTime(0), sendingFileSet(0) {
	appDataSaver = new DataSaver(&appData, sizeof(appData), appDataFilename);
}

// allow us to be found using polling
// via Simple Service Discovery Protocol

void WebHandler::setupSSDP() {
	Serial.printf("Starting SSDP...\n");
	SSDP.setSchemaURL("description.xml");
	SSDP.setHTTPPort(80);
	char namebuf[160];
	snprintf(namebuf, sizeof(namebuf), "%s (%s)", ident.getType(), ident.getLocation());	//e.g. "HAW Temperature Monitor (kitchen)"
	SSDP.setName("Temperature Sensor");
	SSDP.setSerialNumber("20261018202423");
	SSDP.setURL("index.html");
	SSDP.setModelName(namebuf);
	SSDP.setModelNumber("20261018");
	//SSDP.setModelURL("http://www.meethue.com");
	SSDP.setManufacturer("HAW Industries");
	//SSDP.setManufacturerURL("http://www.philips.com");
	SSDP.begin();
}

bool WebHandler::saveAppData() {
	appData.calib = g_calib;
	return the_switch_controller->saveSwitchData() && appDataSaver->write();
}

bool WebHandler::restoreAppData() {
	if (! the_switch_controller->restoreSwitchData()) return false;
	if (! appDataSaver->read()) return false;
	g_calib = appData.calib;
	return true;
}



static const char debug_page_form_extra[] PROGMEM = R"=====(
<hr/>
<b>App data:</b><br/>
Temperature: %.3f &deg;C, &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Delay: %u msecs
)=====";

//static char debug_page_form_ender[] PROGMEM = R"=====(
//  <input class="button" type="submit" value="Set" />
//</form>
//)=====";

void WebHandler::renderAppDebugInfo() {
	WiFiClient client = httpServer.client();
	float tmp = the_temperature_logger.getLastTemperature();
	unsigned int del = the_temperature_logger.getLastDelayTime();
	client.printf_P(debug_page_form_extra, tmp, del);
}



// ----------------------------------- HTTP support code -----------------------------------

// shorthand
#define CP(s) client.print(s)
#define CPN(s) client.println(s)
#define CPF(s) client.print(F(s))
#define CPNF(s) client.println(F(s))


static char *std_page_header PROGMEM = R"=====(
<!DOCTYPE HTML>
<html lang="">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>%s</title>
</head>
<body>
<h2>%s</h2>
)=====";

void send_std_header(WiFiClient &client, const char *title) {
	client.printf_P(std_page_header, title, title);
}

// -----------------------------------------------------------------------------------------
// temperature page handler

static char temperature_page_header[] PROGMEM = R"=====(
<table>
<tr><th colspan=4>Current&nbsp;temperature:&nbsp;%s&deg;C at&nbsp;%s</th></tr>
<tr><td>&nbsp;</td></tr>
<tr>
<td style="text-align:left"><a href="graph.html">Graph</a></td>
<td style="text-align:center"><a href="config.html">Settings</a></td>
<td style="text-align:center"><a href="switch.html">Switch</a></td>
<td style="text-align:center"><a href="networkdata.html">Network</a></td>
<td style="text-align:right"><a href="debug.html">Debug</a></td>
</tr>
<tr><td>&nbsp;</td></tr>
<tr><td colspan=4 style="text-align:center">
<div>
  <canvas id="chart"></canvas>						<!--  style="width:340px; height:170px;" -->
</div>
</td></tr></table>
<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
<script>

var ctx = document.getElementById('chart');
var dataplot = new Chart(ctx, {
    type: 'bar',
    data: {
        labels: [],
        datasets: [{
            label: 'Temperature',
            data: [],
            backgroundColor: [
                'rgba(54, 162, 235, 0.2)'
            ],
            borderColor: [
                'rgba(54, 162, 235, 1)'
            ],
            borderWidth: 1
        }]
    },
    options: {
		plugins: {
			legend: {
				display: false
			}
		},
		aspectRatio: 1.3,
		responsive: true,
        scales: {
            y: {
                beginAtZero: false
            }
        }
    }
});

function addData(label, data) {
    dataplot.data.labels.push(label);
    dataplot.data.datasets[0].data.push(data);
}
)=====";

static char temperature_page_trailer[] PROGMEM = R"=====(
dataplot.update();
</script>
</body>
</html>
)=====";

static const int MAXRETURNS = 30;

void WebHandler::temperaturesSendPage() {
	WiFiClient &client = sendHeaderToClient();
	MSGV("sending temperatures page");

	String title = String("Temperature Logger for: ") + WiFi.hostname();
	send_std_header(client, title.c_str());				// may need to add .js retrieval
	char tbuff[20];
	NTPtimeManager.getTimeString(20, tbuff, NTPtimeManager.getAdjustedTime(cg_tzone, cg_dst, cg_offset));
	client.printf_P(temperature_page_header, the_temperature_logger.getLastTemperatureAsString(g_calib/10.0), tbuff);
	static qentry qearr[MAXRETURNS];
	int maxpts = 1440, nskip = 20, nmax;
	for (;;) {
		nmax = maxpts < MAXRETURNS ? maxpts : MAXRETURNS;
		std::tuple<int, int, int> rdata = gatherData(qearr, maxpts, nmax, nskip);
		int ngot = std::get<0>(rdata);
		int foundpts = std::get<1>(rdata);
		int at_end = std::get<2>(rdata);
		MSGV("gathered %d points representing %d, at_end: %d", ngot, foundpts, at_end);
		for (int p = 0; p < ngot; p++) {
			time_t t = qearr[p].ptime;
			t = NTPtime::adjustTime(t, cg_tzone, cg_dst, cg_offset);
			struct tm *tmptr = gmtime(&t);
			char tbuff[20];
			strftime(tbuff, 20, "%H:%M", tmptr);
			client.printf("addData('%s', '%0.2f');\n", tbuff, qearr[p].temp);
		}
		if (foundpts >= maxpts || at_end) break;
		maxpts -= foundpts;
	}
	MSGV("finished adding data");
	//client.printf("addData('12:00', '20.0');\n");
	client.printf_P(temperature_page_trailer);
	MSGV("sent trailer");
}

// End temperature page handler ^^^

// -----------------------------------------------------------------------------------------
// config & messages page


static char config_page_header[] PROGMEM = R"=====(
<script src="https://cdn.jsdelivr.net/npm/htmx.org@2.0.11/dist/htmx.min.js" 
          integrity="sha384-2OatzQy1H+Zd/IIrjr1TcuDGqLXeHhbooAyJY1KdQMKnr4LZ22k31GBLdYKHmVjg"
          crossorigin="anonymous"></script>
<style>input, button { font-family: Arial,Helvetica,sans-serif; font-size:smaller; }</style>
)=====";

static char config_section_header[] PROGMEM = R"=====(
<b>%s</b> &nbsp;&nbsp;&nbsp;&nbsp;
<small>
)=====";

static char config_section_trailer[] PROGMEM = R"=====(
</small>
<hr/>
)=====";



static char timeInfoSection[] PROGMEM = R"=====(
<style scoped="">#toast.htmx-settling { opacity: 100; } #toast { background: #E1F0DA; opacity: 0; transition: opacity 3s ease-out; }</style>
<form style="display: inline;" hx-get="/settimeparameters" hx-target="this" hx-swap="outerHTML settle:3s">
Universal:&nbsp;%s &nbsp;&nbsp;&nbsp;&nbsp; Local:&nbsp;%s
<br/>
<label for="tzone" title="Specify the timezone in hours ahead of GMT">Time zone:
<input type="number" value="%d" id="tzone" name="tzone" min="-12" max="12" size="4"></label>
&nbsp;&nbsp;&nbsp;&nbsp;
<label for="dst" title="Enable DST if you want to adjust by an hour between the second Sunday in March and the first in November">DST:
<input type="checkbox" %s id="dst" name="dst"></label>
&nbsp;&nbsp;&nbsp;&nbsp;
<label for="offset" title="Choose an offset of up to 10 minutes if your preferred personal time is not exactly standard">Time offset:
<input type="number"  value="%d" id="offset" name="offset" min="-10" max="10" size="4"></label>
&nbsp;&nbsp;&nbsp;&nbsp;
<button class="btn" type="submit" title="Set the specified time parameters">Submit</button>
<button class="btn" hx-get="/resettimeparameters" title="Revert the time parameters">Cancel</button>
&nbsp;
</form>
<span id="toast" hx-swap-oob="true"> %s </span>
)=====";


static char fadingTestCode[] PROGMEM = R"=====(
<form id="ft-test" hx-get="/testbutton" hx-swap="innerHTML settle:3s" hx-target="#toast2">
<input type="submit" value="see fading text" class="btn primary">
    <output id="toast2"></output>
</form>
<style scoped="">#toast2.htmx-settling { opacity: 100; } #toast2 { background: #E1F0DA; opacity: 0; transition: opacity 3s ease-out; }</style>
<output id="toast2"></output>
<hr/>
)=====";


static char temperatureInfoSection[] PROGMEM = R"=====(
<span hx-target="this" hx-swap="outerHTML">
Raw:&nbsp;%s&deg;C &nbsp;&nbsp;&nbsp;&nbsp; Offset temp:&nbsp;%s&deg;C
<br/>
<label for="calibbtn" title="Choose a temperature offset of +/- 10.0 degrees if the temperature reading needs to be calibrated to a standard">
Temperature offset: %0.1f &nbsp;&nbsp;
</label>
<button hx-get="/changetemperatureoffset" class="btn" title="Click to change the temperature offset">Click To Change</button>
</span>
)=====";

static char temperatureOffsetChangeForm[] PROGMEM = R"=====(
<span id="tmpdata" hx-target="this" hx-swap="outerHTML">
Raw:&nbsp;%s&deg;C &nbsp;&nbsp;&nbsp;&nbsp; Offset temp:&nbsp;%s&deg;C
<br/>
<form hx-get="/settemperatureoffset" hx-target="#tmpdata" hx-swap="outerHTML">
<label for="calibbtn" title="Choose a temperature offset of +/- 10.0 degrees if the temperature reading needs to be calibrated to a standard">
Temperature offset: &nbsp;&nbsp;
<input type="number"  value="%0.1f" id="calib" name="calib" min="-10" max="10" step="0.1" size="6">
</label>
&nbsp;&nbsp;
<button class="btn" type="submit" title="Set the specified temperature offset">Submit</button>
<button class="btn" hx-get="/resettemperatureoffset" title="Revert the temperature offset">Cancel</button>
</form>
)=====";

void WebHandler::configSendPage() {
	WiFiClient &client = sendHeaderToClient();
	MSGV("sending configuration page");
	String title = String("Configuration for: ") + WiFi.hostname();
	send_std_header(client, title.c_str());
	client.printf_P(config_page_header);

	client.printf_P(config_section_header, "Times");
	sendTimeInfo(client, timeInfoSection, "init");
	client.printf_P(config_section_trailer);
	//client.printf_P(fadingTestCode);

	client.printf_P(config_section_header, "Temperature");
	sendTemperatureInfo(client, temperatureInfoSection);
	client.printf_P(config_section_trailer);

	client.printf_P(config_section_header, "Switch control");
	the_switch_controller->configSendSwitchSection(client);			// <h3>Fan control (tbd)</h3><hr/>
	client.printf_P(config_section_trailer);

	client.printf_P(config_section_header, "Messages");
	sendMessagesInfo(client);
	int lw = sendMessagesData(client);
	CPNF("</body>");
	CPNF("</html>");
	//CPNF("lines written: "); CP(lw);
}

void WebHandler::sendTimeInfo(WiFiClient &client, char infodata[], const char *toast) {
	//MSGV("unix time: %llu", NTPtimeManager.getUnixTime());
	//MSGV("locl time: %llu", NTPtimeManager.getAdjustedTime(g_tzone, g_dst, g_offset));
	char tmsgbufgmt[40], tmsgbuflcl[40];
	NTPtimeManager.getTimeString(40, tmsgbufgmt, NTPtimeManager.getEpoch());
	NTPtimeManager.getTimeString(40, tmsgbuflcl, NTPtimeManager.getAdjustedTime(cg_tzone, cg_dst, cg_offset));
	//MSGV("calib offset: %d", g_calib);
	client.printf_P(infodata,
			tmsgbufgmt, tmsgbuflcl,
			cg_tzone, cg_dst ? "checked" : "", cg_offset,
			toast);
}

void WebHandler::setTimeParameters() {
	WiFiClient &client = sendHeaderToClient();
	MSGV("sending configuration time info after setting");
	bool good = setTimeModifiersFromServer();
	if (good) {
		MSGV("*set* local time modifiers set successfully");
	} else {
		MSGE("*set* failed setting local time modifiers");
	}
	// common_globals.save();					// saved by setTimeModifiersFromServer()
	sendTimeInfo(client, timeInfoSection, good ? "setting parameters successful" : "setting parameters failed");
}

void WebHandler::resetTimeParameters() {
	WiFiClient &client = sendHeaderToClient();
	MSGV("*reset* cancelled changing & reverted configuration time info data");
	sendTimeInfo(client, timeInfoSection, "parameters reset");
}

void WebHandler::getTestButton() {
	WiFiClient &client = sendHeaderToClient();
	client.printf("\nfading test text\n");
}

void WebHandler::configSaveData() {
	return;
	MSGV("saving configuration data");
	WiFiClient &client = sendHeaderToClient(false);
	String action;
	for (uint8_t i=0; i<httpServer.args(); i++) {
		if (httpServer.argName(i).equals("subaction")) action = httpServer.arg(i);
	}
	MSGV("config_save_data, action: %s", action.c_str());

	bool good = true;
	if (action.equals("Set local time modifiers")) {
		good = setTimeModifiersFromServer();
		if (good) {
			CPNF("local time modifiers set successfully");
		} else {
			CPNF("failed setting local time modifiers");
		}
//	} else if (action.equals("Set offset")) {
//		if (! httpServer.hasArg("calib")) good = false;
//		String calib = httpServer.arg(String("calib"));
//		g_calib = (int)10*atof(calib.c_str());
//		if (good) {
//			saveAppData();
//			CPNF("temperature calibration offset set successfully");
//		} else {
//			CPNF("failed setting temperature calibration offset");
//		}
	} else {
		MSGW("unrecognised action '%s' in config_save_data", action.c_str());
		CPN("");
	}

	common_globals.save();
	//config_data_save_to_filesystem();
	//send_reload_reply("/config.html");
}


void WebHandler::sendTemperatureInfo(WiFiClient &client, char infodata[]) {
	char tmpbufraw[10], tmpbufcal[10];
	strcpy(tmpbufraw, the_temperature_logger.getLastTemperatureAsString());
	strcpy(tmpbufcal, the_temperature_logger.getLastTemperatureAsString(g_calib/10.0));
	client.printf_P(infodata, tmpbufraw, tmpbufcal, g_calib/10.0);
}

void WebHandler::changeTemperatureOffset() {
	WiFiClient &client = sendHeaderToClient();
	MSGV("sending configuration offset change section");
	sendTemperatureInfo(client, temperatureOffsetChangeForm);
}

void WebHandler::resetTemperatureOffset() {
	WiFiClient &client = sendHeaderToClient();
	MSGV("sending configuration offset reset section");
	sendTemperatureInfo(client, temperatureInfoSection);
}

void WebHandler::setTemperatureOffset() {
	WiFiClient &client = sendHeaderToClient();
	MSGV("sending configuration offset set section");
	if (httpServer.hasArg("calib")) {
		String calib = httpServer.arg(String("calib"));
		g_calib = (int)10*atof(calib.c_str());
		saveAppData();
		MSGV("temperature calibration offset set successfully to %0.1f", g_calib/10.0);
	} else {
		MSGV("failed setting temperature calibration offset");
	}
	sendTemperatureInfo(client, temperatureInfoSection);
}


void WebHandler::sendMessagesInfo(WiFiClient &client) {
	client.print(F("<br/>\n"));
	// line 1
	long uptime = NTPtimeManager.getUptime();
	if (uptime == 0) {
		CPF("Up for: "); CP(millis());
	} else {
		char tmbuff[20];
	    CPF("Up since: "); CP(NTPtimeManager.getTimeString(20, tmbuff, uptime)); CPF(" UTC ["); CP(millis()); CPF(" ms]");
	}
	CPF(",&nbsp;&nbsp; FreeHeap: "); CP(ESP.getFreeHeap()); CPF("/"); CP(initialFreeHeapSize);
	CPF("<br/>\n");
	// line 2
	CPF("Connected as: &nbsp;&nbsp;"); CP(WiFi.hostname().c_str());
	CPF("&nbsp;&nbsp; to: &nbsp;&nbsp;"); CP(WiFi.SSID()); CPF("&nbsp;&nbsp;at: &nbsp;&nbsp;"); CP(WiFi.localIP());
	CPF(",&nbsp;&nbsp;&nbsp;&nbsp;   signal strength: "); CP(WiFi.RSSI());
	//CPNF("</h5>"); CPNF("<h4>");

	// Note: tstore count & dropped temperatures are for when we do graphing
	// CPF("Stored: "); CP(tstore.count()); CPF(", ");
	CPF("<br/>\n");
	// line 3
	CPF("Pending in store: "); CP(the_temperature_logger.getStore().availableInStore());
	CPF("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<button hx-get=\"/updatemessages\" hx-target=\"#logmsgs\" hx-swap=\"outerHTML\">Update messages</button><div style=\"line-height: 0.70;\"><br/></div>");
	CPF("\n</small>\n");
	//if (dropped_temperatures != 0) {
	//  CPF(",&nbsp;&nbsp;   Dropped: "); CP(dropped_temperatures);
	//}
	//CPNF("</h5>");
/*	- buttons section
	  CPNF("<h3><table><tr>");
	  CPNF("<td><form autocomplete="off" action=\"http:flushmessages.html\" title=\"Clear out log messages\" method=get><input type=submit value=\"Flush\"></form></td>");
	  CPNF("<td><form autocomplete="off" action=\"http:synctostore.html\" title=\"Write out stored data to SPIFFS\" method=get><input type=submit value=\"Sync\"></form></td>");
	  CPNF("<td><form autocomplete="off" action=\"http:listspiffs.html\" title=\"List SPIFFS files to log\" method=get><input type=submit value=\"List\"></form></td>");
	  CPNF("<td><form autocomplete="off" action=\"http:fsdebug.html\" title=\"Open SPIFFS file get/put page\"method=get><input type=submit value=\"Files\"></form></td>");
	  CPNF("<td><form autocomplete="off" action=\"http:togglefancontrol.html\" title=\"Enable/disable fan controller\"method=get><input type=submit value=\"Fan\"></form></td>");
	  CPNF("<td><form autocomplete="off" action=\"http:togglefanstate.html\" title=\"Toggle fan on/off\"method=get><input type=submit value=\"Test\"></form></td>");
	  CPNF("</td></tr></table></h3>");
*/
	//CPNF("<p>");
}

// print to client up to MAXLOGLINESSHOWN lines, first from log file 1 & then log file 0

static int countLinesInLogFile(int n) {
	int linecnt = 0;
	File f = open_message_log_file(n);
	while (f.find('\n')) linecnt += 1;
	f.close();
	return linecnt;
}

// note: the offset of 18 was determined by experiment using firefox and may vary between browsers

static char scrollLogToBottom[] PROGMEM = R"=====(
<script language="javascript">
    function scrollLogToBottom() {
        logTa = document.getElementById("logmsgs");
        logTa.scrollTop = logTa.scrollHeight;
		logTa.scrollTop = logTa.scrollTop - 18;
    }
    scrollLogToBottom();
</script>
)=====";

#define MAXLOGLINELEN 160
#define MAXLOGLINESSHOWN 40

int WebHandler::sendMessagesData(WiFiClient &client) {
	CPNF("<textarea readonly id=\"logmsgs\" autocomplete=\"off\" name=\"textbox\" cols=\"120\" rows=\"11.5\">");		// yields 12 lines in box
	int f0lines = countLinesInLogFile(0), f1lines = 0, lr = 0, lw1 = 0, lw0 = 0;
	//CPF("f0lines: "); CPN(f0lines);
	File f;											// first count lines in log file 0 as f0lines
	char b[MAXLOGLINELEN+1];						// then print up to 50 - f0lines from log file 1
	if (f0lines < MAXLOGLINESSHOWN) {
		f1lines = countLinesInLogFile(1);
		f = open_message_log_file(1);
		while (f.available() > 0) {
			size_t len = f.readBytesUntil('\n', b, MAXLOGLINELEN);
			if (len == 0) break;
			b[len] = '\0';
			if (b[len-1] == '\r') b[len-1] = '\0';
			if (lr++ >= f1lines - (MAXLOGLINESSHOWN - f0lines)) {
				/*CP(lw); CPNF("  |  ");*/ CPN(b); lw1++;
			}
		}
		f.close();
	}
	f = open_message_log_file();					// finally print the balance (up to 50 lines) from log file 0
	lr = 0;
	while (f.available() > 0) {
		size_t len = f.readBytesUntil('\n', b, MAXLOGLINELEN);
		if (len == 0) break;
		b[len] = '\0';
		if (b[len-1] == '\r') b[len-1] = '\0';
		if (lr++ >= (f0lines - MAXLOGLINESSHOWN)) {
			/*CP(lw); CPNF("  ^  ");*/ CPN(b); lw0++;
		}
	}
	f.close();
	CPNF("</textarea>");
	client.printf_P(scrollLogToBottom);
	MSGV("sent %d + %d log lines from f1: %d, f0: %d (%d)", lw1, lw0, f1lines, f0lines, countLinesInLogFile(0));
	return lw1 + lw0;
}

void WebHandler::updateMessages() {
	WiFiClient &client = sendHeaderToClient();
	MSGV("updating messages in textarea");
	int r = sendMessagesData(client);
	MSGV("sent %d log lines", r);
}

void WebHandler::syncBuffer() {
	the_temperature_logger.getStore().flush();
	httpServer.send(200, "text/plain", "OK\n");
}

// list file system contents raw to client
// note: doesn't seem to be any way of get a count of # of files

void WebHandler::listFilesToClient() {
	MSGV("Listing files to client");
	WiFiClient client = httpServer.client();
	client.println("HTTP/1.1 200 OK");
	client.println("Content-Type: text/plain");
	client.println("Connection: close");
	client.println();

	struct FSInfo filesysinfo;
	if (! FileSys.info(filesysinfo)) {
		MSGE("getting file system info failed");
		client.println("No file system info available");
		return;
	}
	int filecnt = 0;
	Dir dir = FileSys.openDir("/");
	char rbuf[80];
	//dir = FileSys.openDir("/");
	while (dir.next()) {                      // List the file system contents
		filecnt += 1;
		sprintf(rbuf, "%ld %s", dir.fileSize(), dir.fileName().c_str());
		client.println(rbuf);
	}
	sprintf(rbuf, "File System: total size: %ld, used: %ld, files: %ld, heap: %ld",
			filesysinfo.totalBytes, filesysinfo.usedBytes, filecnt, ESP.getFreeHeap());
	client.println(rbuf);
}

// send contents of a binary time-temperature file to client
// usable from curl

void WebHandler::getRawFile() {
	String path = httpServer.arg("f");
	if (! sendRawFile(path.c_str())) {
		httpServer.send(404, "text/plain", "File not found\n");
	}
}

void WebHandler::getRawFileChecksum() {
	uint32_t crc = sendingChecksum.finalize();
	MSGV("sending crc32 checksum: %08lx", crc);
	char buf[20];
	sprintf(buf, "%08lX", crc);
	httpServer.send(200, "text/plain", buf);
}

#define ENTRIESPERCHUNK 200       // =2400 bytes, static as 200 entries is too much for stack

bool WebHandler::sendRawFile(const char *path) {				// formerly "load_from_flash"
	if (sendingFileSet) return false;							// a file is already being sent
	MSGV("sending raw file: '%s'", path);
//	// first a test:
//	const char* s = "123456789";
//	sendingChecksum.reset();
//	while (*s) {
//		sendingChecksum.update((uint8_t)*s++);
//	}
//	MSGV("CRC32 test result: %08lX", sendingChecksum.finalize());
	sendingClient = httpServer.client();
	//MSGV("assigned client");
	const char *xpath = path;
	if (path[0] == '/') xpath = &path[1];
	int xlen = strlen(xpath);
	Dir dir = FileSys.openDir("/");
	//MSGV("opened directory");
	while (1) {                      					// traverse file system contents
		//MSGV("about to call dir.next()");
		int r = dir.next();
		//MSGV("result is %d", r);
		//Serial.println("called dir.next");
		if (! r) break;
		//MSGV("about to check %s (%d)", dir.fileName().c_str(), strlen(dir.fileName().c_str()));
		//const char *fnm = dir.fileName().c_str();				// not safe for unknown reasons
		char fnm[80];
		strncpy(fnm, dir.fileName().c_str(), 80);
		fnm[80] = '\0';
		//MSGV("checking %s against %s", xpath, fnm);
		if (strcmp(xpath, fnm) == 0) {							// exact match required
			MSGV("found exact match to %s", xpath);
			int len = dir.fileSize();
			send_octets_preamble(sendingClient, dir.fileSize());
			sendingFile = FileSys.open(dir.fileName(), "r");
			sendingChecksum.reset();
			sendingFileSet = true;
			sendingFileTime = millis();
			MSGV("ready to send");
			return true;
		} else {
			//MSGV("match rejected");
		}
	}
	return false;
}

// the job is done when a fixed number of bytes equal to the file length have been sent

static const int millisWait = 40;

void WebHandler::sendNextFileChunk() {
	if (! sendingFileSet) return;			// no file is being sent
	if (millis() - sendingFileTime < millisWait) return;
	sendingFileTime = millis();
	static qentry buf[ENTRIESPERCHUNK];
	unsigned int bytesread = sendingFile.readBytes((char*)&buf, ENTRIESPERCHUNK*sizeof(qentry));
	if (bytesread != 0) {
		sendingChecksum.update((uint8_t*)buf, bytesread);
		int byteswrote = sendingClient.write((char*)&buf, bytesread);			// for binary writes
		//MSGV("sent %d of %d bytes from %d at %u", byteswrote, bytesread, sendingFile.position(), millis());
		if (byteswrote != bytesread) {
			MSGE("sent only %d of %d bytes from %d at %u", byteswrote, bytesread, sendingFile.position(), millis());
		}
	} else {
		sendingClient.flush();
		sendingClient.stop();
		MSGV("stopped connection after %d bytes (crc: %08lX) at %u", sendingFile.position(), sendingChecksum.finalize(), millis());
		sendingFile.close();
		sendingFileSet = false;
	}
}

void WebHandler::send_octets_preamble(WiFiClient &client, unsigned int len) {
	MSGV("sending preamble (%u)", len);
	int r = client.printf_P(PSTR("HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\n"));
	MSGV("sent %d bytes of line one", r);
	r = client.printf_P(PSTR("Content-Length: %d\r\nConnection: Close\r\n"), len);
	MSGV("sent %d bytes of line two", r);
	client.println();
	MSGV("sent line three");
}
