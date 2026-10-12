/*
 * RotatingFileAppender.cpp
 *
 * Based on RollingFileAppender in log4Esp
 * (https://github.com/hunsalz/log4Esp/blob/master/src/appender/RollingFileAppender.cpp)
 *
 * Doesn't use seek as in some implementations these combined with writes leave garbage in files (\377 chars)
 *
 *  Created on: Oct. 17, 2021
 *      Author: Danny
 */



#include "RotatingFileAppender.h"


namespace log4Esp {

RotatingFileAppender::RotatingFileAppender(const char *fileName,
		  	  	  	  	  	  	  	  	   uint16_t maxFileSize,
										   uint16_t maxBackupIndex,
										   bool addDefaultFormatter) {

	_maxFileSize = maxFileSize;
	_maxBackupIndex = maxBackupIndex;
	_maxBackupIndexWidth = _maxBackupIndex == 0 ? 1 : log10((float)_maxBackupIndex)+1;

	char *fnbuff = (char*)malloc(strlen(fileName)+2);
	sprintf(fnbuff, "%s%s", fileName, fileName[0] == '/' ? "" : "/");		// force prefix with / if necessary
	_fileName = fnbuff;

	if (addDefaultFormatter) setFormatter(Appender::getDefaultFormatter());

	// load file system by default & log the result (but without formatter)
	if (FileSys.begin()) {
		_log.verbose(F("File system mounted."));
	} else {
		_log.warning(F("Mounting file system failed."));
	}
}


void RotatingFileAppender::begin(Level level, const char *msg, va_list *args) { getFile(); }

void RotatingFileAppender::end(Level level, const char *msg, va_list *args) {
	getOutput().write("\n");
	getOutput().flush();
	if (getFile().size() >= getMaxFileSize() && ! _rolling) rollover();
}


void RotatingFileAppender::rollover() {
	_rolling = true;								// so we can output log messages during rollover
	_log.verbose(F("Rolling over log files"));
	bool r;
	int nmlen = strlen(getFileName()) + _maxBackupIndexWidth+1;
	char fnbuff1[nmlen], fnbuff2[nmlen];
	if (getMaxBackupIndex() > 0) {
		sprintf(fnbuff1, "%s.%d", getFileName(), getMaxBackupIndex());
		if (FileSys.exists(fnbuff1)) {
			r = FileSys.remove(fnbuff1);
			if (r) _log.verbose(F("removed %s"), fnbuff1);
			else _log.warning(F("removing %s unsuccessful!"), fnbuff1);
		}
		for(int i = getMaxBackupIndex(); i > 1; i--) {
			sprintf(fnbuff1, "%s.%d", getFileName(), i-1);
			sprintf(fnbuff2, "%s.%d", getFileName(), i);
			if (FileSys.exists(fnbuff1)) {
				r = FileSys.rename(fnbuff1, fnbuff2);
				if (r) _log.verbose(F("renamed %s to %s"), fnbuff1, fnbuff2);
				else _log.verbose(F("renaming %s to %s unsuccessful!"), fnbuff1, fnbuff2);
			}
		}
	}
	closeFile();									// next log message will reopen (hopefully new) log file
	sprintf(fnbuff2, "%s.%d", getFileName(), 1);
	r = FileSys.rename(getFileName(), fnbuff2);
	if (r) _log.verbose(F("renamed %s to %s"), getFileName(), fnbuff2);
	else _log.verbose(F("renaming %s to %s unsuccessful!"), getFileName(), fnbuff2);
	_rolling = false;
}


File RotatingFileAppender::getFile() {

	if (!_file && getFileName()) {
		// open existing file and set file preferences
		if (FileSys.exists(getFileName())) {
			_file = FileSys.open(getFileName(), "a");
			if (_file) {
				_log.trace(F("Open log file [%s] successful."), getFileName());
			} else {
				_log.error(F("Open log file [%s] failed."), getFileName());
			}
			_file.write("\r\n");		// just so we can find where the new log starts easily

		// create a new file and set default file preferences
		} else {
			_file = FileSys.open(getFileName(), "w");
			if (_file) {
				_log.trace(F("Creating new log file [%s] was successful."), getFileName());
			} else {
				_log.error(F("Creating new log file [%s] failed."), getFileName());
			}
		}
	}
	return _file;
}


} // namespace log4Esp




