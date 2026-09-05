/*
 * CrashLog.h
 *
 *  Created on: Sep. 3, 2026
 *      Author: Danny
 */

#ifndef COMMON_SRC_CRASHLOG_H_
#define COMMON_SRC_CRASHLOG_H_

/*
 * For Arduino version 3.0.2 library. 3.1.x may make things a little easier
 */

#include <Arduino.h>
#include <LittleFS.h>


struct CrashCheckpoint {
    uint32_t magic;              // to validate structure
    uint32_t version;            // for future changes

    uint32_t uptime_ms;          // millis() at last checkpoint
    uint32_t uptime_s;           // uptime_ms / 1000

    uint32_t heap_free;          // free heap at checkpoint
    uint32_t heap_low_water;     // lowest observed free heap

    uint32_t app_state_flags;    // user-defined bitfield
    uint32_t app_state_value;    // user-defined numeric state

    uint32_t rtc_time;           // optional: UNIX time if available, else 0

    char     app_state_text[64]; // small textual state (no String)
    char	 crash_reason[32];	 // text reason for last reboot from ESP.getResetReason()
};


static uint32_t g_heap_low_water = 0xFFFFFFFF;

void CrashCheckpoint_init();

void CrashCheckpoint_periodic(uint32_t app_flags,
                              uint32_t app_value,
                              const char *app_text,
                              uint32_t rtc_unix_time);


#endif /* COMMON_SRC_CRASHLOG_H_ */
