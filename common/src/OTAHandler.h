/*
 * OTAHandler.h
 *
 *  Created on: Sep. 10, 2019
 *      Author: Danny
 */

#ifndef OTAHANDLER_H_
#define OTAHANDLER_H_

class OTAHandler {

public:
	OTAHandler();
	void setup(const char *publicname, const char *otapasswd);
	void loop();
};




#endif /* OTAHANDLER_H_ */
