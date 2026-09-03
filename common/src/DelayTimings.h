/*
 * damper-control.h
 *
 *  Created on: Aug. 18, 2021
 *      Author: Danny
 */

#ifndef DELAYTIMINGS_H_
#define DELAYTIMINGS_H_


void set_delay_time(int);		// defined in damper-control.cpp
int get_delay_time();
void init_timings();
int get_timingdatum(int n);
void record_time_diff();

#endif /* DELAYTIMINGS_H_ */
