/*
 * switch_controller.h
 *
 *  Created on: Nov. 3, 2026
 *      Author: Danny
 */

#ifndef SRC_SWITCH_CONTROLLER_H_
#define SRC_SWITCH_CONTROLLER_H_

class SwitchController
{
public:
	SwitchController();
	void setupSwitch();
	void loopSwitch();

private:
	int msgcountin, msgcountout;			// reporting variables
	String switchIPAddress;   				// the switch ip address once we've found it, otherwise ""

	void setupMulticastUDP();				// called from setup
	void checkForMultiscanUdpMessages();	// called from loop
	bool checkForMultiscanUdpMessage();
	void processIncomingPacket(String &incomingPacket, String &loc);
	String getDeviceFriendlyName(String &loc);

	void perhapsAskForSwitchAddress();
};

extern class SwitchController *the_switch_controller;	// defined in temp_logger_main.cpp

#endif /* SRC_SWITCH_CONTROLLER_H_ */
