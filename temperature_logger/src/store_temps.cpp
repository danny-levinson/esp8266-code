/*
 * store_temps.cpp
 *
 *  Created on: Oct. 6, 2026
 *      Author: Danny
 */

#include <Arduino.h>

#include "NTPtime.h"
#include "SerialDebugHelper.h"
#include "MsgLogHandler.h"

#include "temperature_filing.h"
#include "store_temps.h"


#define TSBUFFERSIZE 10                   // write to disk every 10 entries (i.e. 10 minutes)
struct qentry tsbuffer[TSBUFFERSIZE];
int tsbuffercnt = 0;

// replace any temperature readings without ntp times

static void adjustMissingDates() {
	if (! NTPtimeManager.isReady()) {
		MSGW("NTP time not yet set when writing out buffer to file!!");
		return;                             // give up! (we're trying every 60 seconds & haven't had a response in 10 mins)
	}
	uint32_t ts = NTPtimeManager.getEpoch();
	int i;
	qentry qe;
	for (i=tsbuffercnt-1; i>=0; i--) {
		qe = tsbuffer[i];
		if (qe.ptime == 0 || ((qe.flags & 3) == 1)) break;
		ts = qe.ptime - 60;                 // adjust to one minute before this reading
	}
	if (i < 0) return;                    // nothing to do
	if (qe.ptime != 0 || ((qe.flags & 3) != 1)) MSGW("odd reading, time is "+String(qe.ptime)+" but flags are "+String(qe.flags));
	MSGV("replacing times in "+String(i+1)+" readings");
	for (; i>=0; i--) {                   // now replace bad readings
		qe = tsbuffer[i];
		if (qe.ptime != 0 || ((qe.flags & 3) != 1)) MSGW("odd reading in replacement phase, time is "+String(qe.ptime)+" but flags are "+String(qe.flags));
		tsbuffer[i].ptime = ts;
		tsbuffer[i].flags = (qe.flags & ~3) | 2;  // replace 1 with 2
		ts -= 60;
	}
}

// real version is in temperature_filing.cpp; this one is just for testing
// note: failing to return a value causes a mystery exception to happen here !!

//bool write_bytes_to_file_store(unsigned char *b, int len) {
//	//MSGV("writing %d bytes to store", len);
//	MSGV("writing "+String(len)+" bytes to store");
//	return false;
//}

static bool flushBufferToFile() {
	if (tsbuffercnt == 0) return true;      // e.g. true on sync after sync
	adjustMissingDates();                   // replace stamps on any temperature readings without ntp times
	int bcnt = tsbuffercnt;
	tsbuffercnt = 0;                        // even if flushing fails!
	MSGV("writing "+String(bcnt)+" temperature records to store");
	if (! write_bytes_to_file_store((unsigned char*)&tsbuffer, bcnt*sizeof(qentry))) {
		MSGW("failed to write buffer to file");
		return false;
	}
	return true;
}

static void saveInBuffer(qentry &qe) {
	if (tsbuffercnt < TSBUFFERSIZE) {         // should never fail!
		tsbuffer[tsbuffercnt++] = qe;
	}
	MSGV("saved entry #"+String(tsbuffercnt)+" to buffer");
	if (tsbuffercnt >= TSBUFFERSIZE) flushBufferToFile();					// filed in file system - ignore error return
}

TemperatureStore::TemperatureStore() {
}

// flags:
//     bottom 2 bits:  0 - old data, 1 - no ntp time, 2 - ntp time available,
//                     3 - time reported is unix ts * 1000
//     bit 3: no ntp refresh for > 1 hour (stale)
// ptime:
//     if flags = 3: unix time + seconds since last unix time retrieval
//     else: millis since start
// Notes: unsigned long is only 2^32 & not long enough for unix time * 1000
//       could maybe experiment with uint64_t instead
//		 ntp time stale flag is dropped in this version

void TemperatureStore::pushReading(float tmp) {
	uint8_t flags = 0;
	unsigned long evtime = 0;		// millis() or ntp time
	if (NTPtimeManager.isReady()) {
		evtime = NTPtimeManager.getEpoch();
		flags = 3;
	} else {
		evtime = millis();
		flags = 1;
	}
	if (DEBUG) {
		MSGV("pushing temperature record of '%f' at %lu", tmp, evtime);
	}
	//if (isSwitchOn()) flags |= 8;				// TBD
	pushRecord(flags, evtime, tmp);
}

void TemperatureStore::pushRecord(uint8_t flags, ulong ptime, float tmp) {
	qentry qe = {ptime, tmp, flags};
	saveInBuffer(qe);                   // save in the little buffer to push to a file shortly
}


int TemperatureStore::availableInStore() {
  return tsbuffercnt;
}

// num *must* be less than or equal to tsbuffercnt

void TemperatureStore::copyFromStore(qentry *qe, int num) {
  for (int i = 0; i < num; i++) qe[i] = tsbuffer[i];
}


