/*
 * CrashLog.h
 *
 *  Created on: Sep. 3, 2026
 *      Author: Danny
 */

#ifndef COMMON_SRC_CRASHLOG_H_
#define COMMON_SRC_CRASHLOG_H_

/*
 * For Arduino version 3.0.2 library. 3.1.x makes things a little easier
 */

#pragma once
#include <Arduino.h>
#include <LittleFS.h>

struct CrashInfo {
    String reason;
    uint32_t lastState;
    uint32_t uptime;
};

class CrashLog {
public:
    static void begin();
    static void recordState(uint32_t state);
    static bool load(CrashInfo &info);
    static void clear();

private:
    static void write(const CrashInfo &ci);
    static void rotateLogs();
    static uint32_t lastState;
};



#endif /* COMMON_SRC_CRASHLOG_H_ */
