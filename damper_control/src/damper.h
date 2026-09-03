/*
 * damper.h
 *
 *  Created on: Aug. 30, 2021
 *      Author: Danny
 */

#ifndef DAMPER_H_
#define DAMPER_H_

class Damper
{
public:
	Damper();

	void set_servo_angles(int, int);
	void get_servo_angles(int&, int&);

	void open_damper();
	void close_damper();

	void loop_damper();
	void setup_damper();

};

extern Damper the_damper;


#endif /* DAMPER_H_ */
