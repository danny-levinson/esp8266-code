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

};

extern class SwitchController *the_switch_controller;	// defined in temp_logger_main.cpp

#endif /* SRC_SWITCH_CONTROLLER_H_ */
