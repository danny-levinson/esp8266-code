/*
 * store_temps.h
 *
 *  Created on: Oct. 6, 2026
 *      Author: Danny
 */

#ifndef SRC_STORE_TEMPS_H_
#define SRC_STORE_TEMPS_H_

struct qentry;		// real declaration is in temperature_filing.h

class TemperatureStore
{
public:
	TemperatureStore();
	void pushReading(float tmp);

	int availableInStore();
	void copyFromStore(qentry *qe, int num);

private:
	void pushRecord(uint8_t flags, ulong ptime, float tmp);
};



#endif /* SRC_STORE_TEMPS_H_ */
