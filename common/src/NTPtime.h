/*
 * NTPtime.h
 *
 *  Created on: Sep. 10, 2019
 *      Author: Danny
 *  Rewritten by copilot as a class derived from its RobustNTPClient, Aug 10, 2026
 */

#ifndef NTP_TIME_H
#define NTP_TIME_H

#include "RobustNTPClient.h"
#include <time.h>


class NTPtime : public RobustNTPClient {
public:
    NTPtime(const char *server1,
            const char *server2 = nullptr,
            uint16_t port = 123);

    uint32_t getUptime();  // UTC epoch when system came up

    static time_t adjustTime(time_t t, int tzone, int dst, int offset);
    time_t getAdjustedTime(int tzone, int dst, int offset);

    static char* getTimeString(int tbuflen, char *tbuff, time_t t);

private:
    uint32_t bootEpoch;    // true epoch at boot time
};

extern NTPtime NTPtimeManager;

#endif
