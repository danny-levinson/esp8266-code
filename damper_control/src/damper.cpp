/*
 * damper.cpp
 *
 *  Created on: Sep. 7, 2021
 *      Author: Danny
 */

#include "damper.h"

#include <ESP8266WiFi.h>

#include <Servo.h>

#include "SerialDebugHelper.h"
#include "DelayTimings.h"

// The Servo library is kept locally in: ~/Library/Arduino15/packages/esp8266/hardware/esp8266/2.5.0/libraries

static Servo myservo;  // create servo object to control a servo

static const int servo_pin = D6;

static int closed_pos = 135;					// need to be saved in file system & made changeable (config file?)
static int open_pos = 50;

static const unsigned long min_delay = 15;
static unsigned long lastmvmttime = 0;

static int curpos = closed_pos;
static int targetpos = closed_pos;

Damper the_damper;

Damper::Damper() {
}

void Damper::set_servo_angles(int p1, int p2) {
	closed_pos = p1;
	open_pos = p2;
}

void Damper::get_servo_angles(int &p1, int &p2) {
	p1 = closed_pos;
	p2 = open_pos;
}

void Damper::open_damper() {
	targetpos = open_pos;
}

void Damper::close_damper() {
	targetpos = closed_pos;
}

void Damper::setup_damper() {
//	myservo.attach(servo_pin);
}

static void maybe_attach_servo() {
	if (myservo.attached()) return;
	myservo.attach(servo_pin);
}

static void maybe_detach_servo() {
	if (! myservo.attached()) return;
	myservo.detach();
}

void Damper::loop_damper() {
	if (curpos == targetpos) {
		maybe_detach_servo();
		return;
	}
	if (millis() <= lastmvmttime + min_delay) return;
	maybe_attach_servo();
	curpos += (curpos < targetpos) ? 1 : -1;
	myservo.write(curpos);
#if DEBUG
	static int n_in_line = 0;
	SPRNTF(1, "%d, ", curpos);
	if (n_in_line++ > 30) { SPRTLN(1, ""); n_in_line = 0; }
#endif
	lastmvmttime = millis();
}



