/*
 * RotatingFileAppender.h
 *
 * Based on RollingFileAppender in log4Esp
 * (https://github.com/hunsalz/log4Esp/blob/master/src/appender/RollingFileAppender.h)
 *
 *  Created on: Oct. 17, 2021
 *      Author: Danny
 */

#ifndef ROTATINGFILEAPPENDER_H_
#define ROTATINGFILEAPPENDER_H_

#include <log4Esp.h>
#include "FileSys.h"


namespace log4Esp {


/**
 * RotatingFileAppender logs data to a rotating set of files.
 */

class RotatingFileAppender : public Appender {

public:
  RotatingFileAppender(const char *fileName,
		  	  	  	  uint16_t maxFileSize = 10 * 1024,
					  uint16_t maxBackupIndex = 1,
                      bool addDefaultFormatter = true);
  virtual ~RotatingFileAppender() {}

  Print &getOutput() { return _file; }
  const char *getFileName() { return _fileName; }

  uint16_t getMaxFileSize() { return _maxFileSize; }
  uint16_t getMaxBackupIndex() { return _maxBackupIndex; }

  void setLogger(Logger logger) { _log = logger; }
protected:
  void begin(Level level, const char *msg, va_list *args);
  void end(Level level, const char *msg, va_list *args);

private:
  const char *_fileName = NULL;
  File _file;
  uint16_t _maxFileSize;
  uint16_t _maxBackupIndex;
  unsigned short int _maxBackupIndexWidth;
  Logger _log = Logger();
  bool _rolling = false;

  void rollover();

  File getFile();
  void closeFile() { _file.close(); }
};


} // namespace log4Esp

#ifndef LOGGER_NO_GLOBALS
using log4Esp::RotatingFileAppender;
#endif // LOGGER_NO_GLOBALS





#endif /* ROTATINGFILEAPPENDER_H_ */
