/*
 * StandardHelpers.h
 *
 *  Created on: Aug. 16, 2026
 *      Author: Danny
 */

#ifndef STANDARDHELPERS_H_
#define STANDARDHELPERS_H_

#include "Arduino.h"
#include "WebHandlerBase.h"
#include "WebSocketBase.h"

class StandardHelpers {
public:
	void setup(WebHandlerBase *webhandler,
			   WebSocketBase *sockethandler,
			   const char *nameaspfx,
			   bool debug);
	void loopBegin();
	void loopEnd();
};



#endif /* STANDARDHELPERS_H_ */
