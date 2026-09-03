/*
 * DelayTimings.cpp
 *
 *  Created on: Sep. 24, 2021
 *      Author: Danny
 */


#include "DelayTimings.h"
#include "FixedQueueArray.h"


static int delay_time = 20;

void set_delay_time(int dt) {
	delay_time = dt;
}

int get_delay_time() {
	return delay_time;
}

// monitor length of time to execute main loop

FixedSizeQueueArray<int> timings(20);		// "timings" is just a list of times

void init_timings() {
	timings.setPrinter(Serial);
}

int get_timingdatum(int n) {
	if (n >= timings.count()) return -100;	// regular values are always positive
	return timings.get(n);
}

void record_time_diff() {
	static long laststarttime = 0;
	long timenow = millis();
	timings.enqueue(timenow - laststarttime);
	laststarttime = timenow;
}
