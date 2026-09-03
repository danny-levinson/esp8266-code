/*
 * NTPtime.cpp
 *
 *  Created on: Sep. 10, 2019
 *      Author: Danny
 *  Rewritten by copilot as a class derived from its RobustNTPClient, Aug 10, 2026
 */
#include "NTPtime.h"

NTPtime NTPtimeManager("time.google.com", "pool.ntp.org");

NTPtime::NTPtime(const char *server1,
                 const char *server2,
                 uint16_t port)
    : RobustNTPClient(server1, server2, port),
      bootEpoch(0)
{
}

uint32_t NTPtime::getUptime() {
    if (bootEpoch == 0 && isReady()) {
        // True boot time = first valid epoch minus uptime seconds
        bootEpoch = getEpoch() - (millis() / 1000);
    }
    return bootEpoch;
}

// North American DST rules with 02:00 cutoff
static bool isDST_NorthAmerica(const struct tm *local) {
    int month = local->tm_mon + 1;   // 1–12
    int wday  = local->tm_wday;      // 0=Sun
    int mday  = local->tm_mday;
    int hour  = local->tm_hour;

    // Before March or after November → no DST
    if (month < 3 || month > 11) return false;

    // April–October → always DST
    if (month > 3 && month < 11) return true;

    // March: DST starts 2nd Sunday at 02:00
    if (month == 3) {
        int firstSunday = (7 - wday + mday) % 7;
        int secondSunday = firstSunday + 7;

        if (mday < secondSunday) return false;
        if (mday > secondSunday) return true;

        // On the exact day → only after 02:00
        return hour >= 2;
    }

    // November: DST ends 1st Sunday at 02:00
    if (month == 11) {
        int firstSunday = (7 - wday + mday) % 7;

        if (mday < firstSunday) return true;
        if (mday > firstSunday) return false;

        // On the exact day → only before 02:00
        return hour < 2;
    }

    return false;
}

time_t NTPtime::adjustTime(time_t t, int tzone, int dst, int offset) {
    // Convert UTC → local time first
    time_t local = t + (tzone * 3600);

    struct tm *tm_local = gmtime(&local);

    // Apply DST only if requested AND in effect
    if (dst && isDST_NorthAmerica(tm_local)) {
        local += 3600;
    }

    // Apply minute displacement
    local += (offset * 60);

    return local;
}

time_t NTPtime::getAdjustedTime(int tzone, int dst, int offset) {
    if (!isReady()) return 0;

    time_t now = (time_t)getEpoch();
    return adjustTime(now, tzone, dst, offset);
}

char* NTPtime::getTimeString(int tbuflen, char *tbuff, time_t t) {
    if (tbuflen < 20) {   // needs 17 chars + null
        if (tbuflen > 0) tbuff[0] = '\0';
        return tbuff;
    }

    struct tm *tm_info = gmtime(&t);

    snprintf(tbuff, tbuflen,
             "%02d-%02d-%02d %02d:%02d:%02d",
             (tm_info->tm_year % 100),
             tm_info->tm_mon + 1,
             tm_info->tm_mday,
             tm_info->tm_hour,
             tm_info->tm_min,
             tm_info->tm_sec);

    return tbuff;
}
