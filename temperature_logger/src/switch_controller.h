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
	SwitchController(class WebHandlerBase *wbh);
	void setupSwitch();
	void loopSwitch();

	void configSendSection(WiFiClient &client);

private:
	int msgcountin, msgcountout;			// reporting variables
	String switchIPAddress;   				// the switch ip address once we've found it, otherwise ""
	class WebHandlerBase *webhandler;		// just to ask if we are connected

	void setupMulticastUDP();				// called from setup
	void checkForMultiscanUdpMessages();	// called from loop
	bool checkForMultiscanUdpMessage();
	void processIncomingPacket(String &incomingPacket, String &loc);
	String getDeviceFriendlyName(String &loc);

	void perhapsAskForSwitchAddress();

	String switchAddress();
	int getSwitchState();
	bool setSwitchState(bool onOrOff);
	int checkSwitchState();
	void unsetSwitchState();
	int getCurrentSwitchState();
	bool isSwitchOn();

	void maybeActuateSwitch();

};

extern class SwitchController *the_switch_controller;	// defined in temp_logger_main.cpp

#endif /* SRC_SWITCH_CONTROLLER_H_ */
