/*
 * MsgLogHandler.h
 *
 *  Created on: Sep. 28, 2021
 *      Author: Danny
 */

#ifndef LOGHANDLER_H_
#define LOGHANDLER_H_

#include <log4Esp.h>


// default is replaced by own logger that does log to console, ota & file, & also timestamps
//#define MSGLOG log4Esp::LOG				// for log4Esp 1.0.1
//#define MSGLOG LOG						// for log4Esp 1.0.2
#define MSGLOG TL_Logger

#define MSGE(...) MSGLOG.error(__VA_ARGS__)
#define MSGW(...) MSGLOG.warning(__VA_ARGS__)
#define MSGV(...) MSGLOG.verbose(__VA_ARGS__)
#define MSGT(...) MSGLOG.trace(__VA_ARGS__)

extern Logger TL_Logger;

void setup_message_logging();
File open_message_log_file(int fno=0);	// for reading


extern int HeapHWM;

#endif /* LOGHANDLER_H_ */
