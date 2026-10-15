/*
 * WebHandler.cpp
 *
 *  Created on: Sep. 29, 2026
 *      Author: Danny
 */

#include "MsgLogHandler.h"
#include "FileSys.h"
#include "NTPtime.h"

#include "WebHandler.h"
#include "DataSaver.h"
#include "CommonGlobals.h"

#include "temperature_logger.h"
#include "temperature_filing.h"
#include "gdata.h"

static float g_calib = 0.0;

void WebHandler::registerAppRoutes() {
	httpServer.on("/index.html", HTTP_GET, [this]() { this->temperaturesSendPage(); });
	httpServer.on("/temperatures.html", HTTP_GET, [this]() { this->temperaturesSendPage(); });
	httpServer.on("/config.html", HTTP_GET, [this]() { this->configSendPage(); });
	httpServer.on("/config", HTTP_GET, [this]() { this->configSendPage(); });
	httpServer.on("/configset", HTTP_GET, [this]() { this->configSaveData(); });

	//httpServer.on("/dbgsetservoangles", HTTP_GET, [this]() { this->debug_set_servo_angles(); });
}


WebHandler::WebHandler(const char *genericname) : WebHandlerBase(genericname) {
	appDataSaver = new DataSaver(&appData, sizeof(appData), "/appconfig");
}


bool WebHandler::saveAppData() {
	appData.calib = g_calib;
	return appDataSaver->write();
}

bool WebHandler::restoreAppData() {
	bool r = appDataSaver->read();
	g_calib = appData.calib;
	return r;
}



static const char debug_page_form_extra[] PROGMEM = R"=====(
<hr/>
Temperature - %.3f &deg;C, &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Delay - %u msecs
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
//	CP(FPSTR(debug_page_form_extra));
//	int ang1, ang2;
//	the_damper.get_servo_angles(ang1, ang2);
//	client.printf("Closed: <input type=\"text\" name=\"ang1\" size=\"4\" value=\"%d\">\r\n", ang1);
//	client.printf("Open: <input type=\"text\" name=\"ang2\" size=\"4\" value=\"%d\">\r\n", ang2);
//	CP(FPSTR(debug_page_form_ender));
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


static char config_page_format[] PROGMEM = R"=====(
<form autocomplete="off" method=GET action="configset">
Universal&nbsp;time:&nbsp;%s &nbsp;&nbsp;&nbsp;&nbsp; Local&nbsp;time:&nbsp;%s
<p/>
<label for="tzone" title="Specify the timezone in hours ahead of GMT">Time zone:
<input type="number" value="%d" id="tzone" name="tzone" min="-12" max="12" size="4"></label>
&nbsp;&nbsp;&nbsp;&nbsp;
<label for="dst" title="Enable DST if you want to adjust by an hour between the second Sunday in March and the first in November">DST:
<input type="checkbox" %s id="dst" name="dst"></label>
&nbsp;&nbsp;&nbsp;&nbsp;
<label for="offset" title="Choose an offset of up to 10 minutes if your preferred personal time is not exactly standard">Time offset:
<input type="number"  value="%d" id="offset" name="offset" min="-10" max="10" size="4"></label>
&nbsp;&nbsp;&nbsp;&nbsp;
<input type="submit" name="subaction" value="Set local time modifiers">
<hr/>
Raw&nbsp;temperature:&nbsp;%s&deg;C &nbsp;&nbsp;&nbsp;&nbsp; Offset&nbsp;temperature:&nbsp;%s&deg;C
<p/>
<label for="calib" title="Choose a temperature offset of +/- 10.0 degrees if the temperature reading needs to be calibrated to a standard">Temperature offset:
<input type="number"  value="%0.1f" id="calib" name="calib" min="-10" max="10" step="0.1" size="6"></label>
&nbsp;&nbsp;&nbsp;&nbsp;
<input type="submit" name="subaction" value="Set offset">
</form>
<hr/>
<h2>Fan control (tbd)</h2>
<hr/>
<h2>Messages</h2>
)=====";

void WebHandler::configSendPage() {
	WiFiClient &client = sendHeaderToClient();
	MSGV("sending configuration page");
	String title = String("Configuration for: ") + WiFi.hostname();
	send_std_header(client, title.c_str());
	//MSGV("unix time: %llu", NTPtimeManager.getUnixTime());
	//MSGV("locl time: %llu", NTPtimeManager.getAdjustedTime(g_tzone, g_dst, g_offset));
	char tmsgbufgmt[40], tmsgbuflcl[40], tmpbufraw[10], tmpbufcal[10];
	NTPtimeManager.getTimeString(40, tmsgbufgmt, NTPtimeManager.getEpoch());
	NTPtimeManager.getTimeString(40, tmsgbuflcl, NTPtimeManager.getAdjustedTime(cg_tzone, cg_dst, cg_offset));
	strcpy(tmpbufraw, the_temperature_logger.getLastTemperatureAsString());
	strcpy(tmpbufcal, the_temperature_logger.getLastTemperatureAsString(g_calib/10.0));
	//MSGV("calib offset: %d", g_calib);
	client.printf_P(config_page_format,
			tmsgbufgmt, tmsgbuflcl,
			cg_tzone, cg_dst ? "checked" : "", cg_offset,
			tmpbufraw, tmpbufcal,
			g_calib/10.0);

	client.print(F("<h5>"));
	long uptime = NTPtimeManager.getUptime();
	if (uptime == 0) {
		CPF("Up for: "); CP(millis());
	} else {
		char tmbuff[20];
	    CPF("Up since: "); CP(NTPtimeManager.getTimeString(20, tmbuff, uptime)); CPF(" UTC ["); CP(millis()); CPF(" ms]");
	}
	CPF(",&nbsp;&nbsp;   FreeHeap: "); CP(ESP.getFreeHeap()); CPF("/"); CP(ESP.getFreeHeap());
	CPF("<br/>");
	CPF("Connected as: &nbsp;&nbsp;"); CP(WiFi.hostname().c_str());
	CPF("&nbsp;&nbsp; to: &nbsp;&nbsp;"); CP(WiFi.SSID()); CPF("&nbsp;&nbsp;at: &nbsp;&nbsp;"); CP(WiFi.localIP());
	CPF(",&nbsp;&nbsp;&nbsp;&nbsp;   signal strength: "); CP(WiFi.RSSI());
	//CPNF("</h5>"); CPNF("<h4>");

	// Note: tstore count & dropped temperatures are for when we do graphing
	// CPF("Stored: "); CP(tstore.count()); CPF(", ");
	CPF("<br/>");
	CPF("Pending in store: "); CP(the_temperature_logger.getStore().availableInStore());
	//if (dropped_temperatures != 0) {
	//  CPF(",&nbsp;&nbsp;   Dropped: "); CP(dropped_temperatures);
	//}
	CPNF("</h5>");
/*
	  CPNF("<h3><table><tr>");
	  CPNF("<td><form autocomplete="off" action=\"http:flushmessages.html\" title=\"Clear out log messages\" method=get><input type=submit value=\"Flush\"></form></td>");
	  CPNF("<td><form autocomplete="off" action=\"http:synctostore.html\" title=\"Write out stored data to SPIFFS\" method=get><input type=submit value=\"Sync\"></form></td>");
	  CPNF("<td><form autocomplete="off" action=\"http:listspiffs.html\" title=\"List SPIFFS files to log\" method=get><input type=submit value=\"List\"></form></td>");
	  CPNF("<td><form autocomplete="off" action=\"http:fsdebug.html\" title=\"Open SPIFFS file get/put page\"method=get><input type=submit value=\"Files\"></form></td>");
	  CPNF("<td><form autocomplete="off" action=\"http:togglefancontrol.html\" title=\"Enable/disable fan controller\"method=get><input type=submit value=\"Fan\"></form></td>");
	  CPNF("<td><form autocomplete="off" action=\"http:togglefanstate.html\" title=\"Toggle fan on/off\"method=get><input type=submit value=\"Test\"></form></td>");
	  CPNF("</td></tr></table></h3>");
*/
	CPNF("<p>");
	int f0lines = 0, f1lines = 0, lr = 0, lw = 0;
	File f = open_message_log_file();
	while (f.find('\n')) f0lines += 1;
	f.close();
	//CPF("f0lines: "); CPN(f0lines);
	CPNF("<textarea name=\"textbox\" cols=\"120\" rows=\"11\">");		// yields 12 lines in box
	char b[120];
	if (f0lines < 50) {
		f = open_message_log_file(1);
		while (f.find('\n')) f1lines += 1;
		f.close();
		f = open_message_log_file(1);
		while (f.available() > 0) {
			size_t len = f.readBytesUntil('\n', b, 120);
			b[len] = '\0';
			lr += 1;
			if (lr > f1lines - (50 - f0lines)) {
				/*CP(lw); CPNF("  |  ");*/ CPN(b); lw++;
			}
		}
	}
	f.close();
	f = open_message_log_file();
	lr = 0;
	while (f.available() > 0) {
		size_t len = f.readBytesUntil('\n', b, 120);
		b[len] = '\0';
		lr += 1;
		if (lr > (f0lines - 50)) {
			/*CP(lw); CPNF("  ^  ");*/ CPN(b); lw++;
		}
	}
	f.close();
	CPNF("</textarea>");
	CPNF("</body>");
	CPNF("</html>");
	//CPNF("lines written: "); CP(lw);
}


void WebHandler::configSaveData() {
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
	} else if (action.equals("Set offset")) {
		if (! httpServer.hasArg("calib")) good = false;
		String calib = httpServer.arg(String("calib"));
		g_calib = (int)10*atof(calib.c_str());
		if (good) {
			saveAppData();
			CPNF("temperature calibration offset set successfully");
		} else {
			CPNF("failed setting temperature calibration offset");
		}
	} else {
		MSGW("unrecognised action '%s' in config_save_data", action.c_str());
		CPN("");
	}

	common_globals.save();
	//config_data_save_to_filesystem();
	//send_reload_reply("/config.html");
}

