/*
 * MsgLogHandler.cpp
 *
 *  Created on: Sep. 28, 2021
 *      Author: Danny
 */

#include <Log4Esp.h>
#include "FileSys.h"

#include "RotatingFileAppender.h"
#include "SerialDebugHelper.h"
#include "DelayTimings.h"
#include "NTPtime.h"
#include "MsgLogHandler.h"


static const char *log_file_name = "/messages.log";


//static String formatBytes(size_t bytes) { // convert sizes in bytes to KB and MB
//  if (bytes < 1024) {
//    return String(bytes) + "B";
//  } else if (bytes < (1024 * 1024)) {
//    return String(bytes / 1024.0) + "KB";
//  } else if (bytes < (1024 * 1024 * 1024)) {
//    return String(bytes / 1024.0 / 1024.0) + "MB";
//  }
//  return "?";
//}

int HeapHWM = 999999;

static char *checkmem() {
	static char mcbuf[40];
	int heap = ESP.getFreeHeap();
	if (heap < HeapHWM) HeapHWM = heap;
	sprintf(mcbuf, "   [%d,%d]", HeapHWM, ESP.getFreeContStack());
	return mcbuf;
}

Logger TL_Logger = Logger("TL_Logger");

static void add_timestamp_formatter_to_logger() {
	// use one of two possible methods to add an Level filter
	// logger.getAppender().at(0).setFomatter(...); // adding Formatter to only one specific Appender
	TL_Logger.addFormatterToAll(
			[](Print &output, Appender::Level level, const char *msg, va_list *args) { // adding formatter to all Appenders

		char tmsgbuflcl[40];
		//strcpy(tmsgbuflcl, NTPtimeManager.getTimeString(NTPtimeManager.getAdjustedTime(g_tzone, g_dst, g_offset), false));
		NTPtimeManager.getTimeString(40, tmsgbuflcl, NTPtimeManager.getEpoch(), false);
		output.print(tmsgbuflcl);
		output.print(F(" "));
		output.print(Appender::toString(level, true));
		output.print(F("| "));
		size_t length = vsnprintf(NULL, 0, msg, *args) + 1;
		char buffer[length];
		vsnprintf(buffer, length, msg, *args);
		output.print(buffer);
		output.print(checkmem());
	});
}

// open log file for reading

File open_message_log_file(int fno) {
	File f;
	if (fno == 0) {
		return FileSys.open(log_file_name, "r");
	}
	if (fno > 0) {
		char cbuff[strlen(log_file_name)+10];
		sprintf(cbuff, "%s.%d", log_file_name, fno);
		return FileSys.open(cbuff, "r");
	}
	return f;
}

//// test to determine that it is use of seek that is causing rubout chars to be inserted in log file
//void write_offset(File &f, int offset) {
//	f.seek(0, SeekSet);
//    char digits[11]; // max offset value consists of 10 digits
//    sprintf(digits, "%-10d", offset);
//    f.print(digits);
//    //f.flush();
//}
//void write_msg(File &f, char *msg) {
//	const int rowlen = 79;
//	int offset = f.position();
//	f.print(msg);
//	int spacing = rowlen - (f.position() - offset) - 1;
//	if (spacing > 0) f.printf("%*s", spacing, "");
//	else { f.seek((spacing - 4), SeekCur); f.print("[..]"); }
//	MSGV("spacing = %d", spacing);
//	f.println();
//	//offset = f.position();
//	//write_offset(f, offset);
//	//f.seek(offset, SeekSet);
//	//delay(1000);
//}
//void run_seek_test() {
//	File f = FileSys.open("/seektest.txt", "w+");
//	//f.seek(0, SeekSet);
//	write_offset(f, 12);
//	f.println();
//	//write_msg(f, "70/01/01 00:00:00 V| Logging started   [32704,2224]");
//	//write_msg(f, "70/01/01 00:00:00 V| Starting WiFi as station, hostname 'furnace'   [32672,2224]");
//	write_msg(f, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
//	//write_msg(f, "70/01/01 00:00:00 V| Setting up OTA   [32040,2224]");
//	//write_msg(f, "70/01/01 00:00:00 V| Starting HTTP   [30944,2224]");
//	f.close();
//}
//
//// see project badwritetest
//void run_small_write_test() {
//	File f = FileSys.open("/writetest.txt", "w+");
//	//f.seek(0, SeekSet);
//	//f.print("12\r\n");//        \r\n");
//	//f.println();
//	f.write("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
//	f.seek(-6, SeekCur);
//	f.print("yyyy\r\n");
//	//f.println();
//	f.close();
//
//	unsigned char buff[120] = "";
//	char cbuff[120] = "";
//	f = FileSys.open("/writetest.txt", "r");
//	f.read(buff, 80);
//	for (int i=70; i<80; i++) {
//		sprintf(cbuff+(i-70)*3, "%02x ", buff[i]);
//	}
//	MSGV("output: |%s|", cbuff);
//	f.close();
//}


void setup_message_logging() {
#if DEBUG==1
//	Serial.begin(115200);
//	Serial.setDebugOutput(false);				// enable/disable wifi state logging
	while (!Serial && !Serial.available()) {
	};
//	delay(300);
//	setup_file_system();
	//FileSys.remove(log_file_name);		// start fresh if needed for testing
	RotatingFileAppender *rfa = new RotatingFileAppender(log_file_name, 10*1024, 3, true);
	TL_Logger.getAppender().push_back(rfa);
	add_timestamp_formatter_to_logger();
	rfa->setLogger(TL_Logger);
	//SPRTLN(1, "");
	//run_seek_test();
	//run_small_write_test();
	init_timings();
#endif
	//Serial.println("hello14...");
	MSGV("Logging started");
}
