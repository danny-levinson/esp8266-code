/*
 * CrashLog.cpp
 *
 *  Created on: Sep. 3, 2026
 *      Author: Danny
 *
 *  Module to periodically checkpoint diagnostic data
 *  and handling logging of same on reboots
 */


#include "CrashLog.h"
#include "FileSys.h"
#include "SerialDebugHelper.h"


static const char *tmplogname = "/crash.log";
static const char *lognamepattern = "/crash-%d.log";


static void CrashLog_rotateLogs() {
	char namebuf1[20], namebuf2[20];
	sprintf(namebuf1, lognamepattern, 9);
	if (FileSys.exists(namebuf1)) {
		FileSys.remove(namebuf1);
	}
	for (int i=8; i>=0; i--) {
		sprintf(namebuf1, lognamepattern, i);
		if (FileSys.exists(namebuf1)) {
			sprintf(namebuf2, lognamepattern, i+1);
			FileSys.rename(namebuf1, namebuf2);
		}
	}
	FileSys.rename(tmplogname, namebuf1);
}

static void CrashLog_writeTmpLog(CrashCheckpoint &rcp) {
    File f = FileSys.open(tmplogname, "w");
    if (!f) return;

    f.write((const uint8_t*)&rcp, sizeof(CrashCheckpoint));
    f.close();
    CrashLog_rotateLogs();
}


static const uint32_t MAGIC = 0x43524348;		// 'CRCH'
static const uint32_t VERSION = 1;

static const char *checkpoint_filename = "/checkpoint.bin";


static void init_checkpoint_struct(CrashCheckpoint *p) {
    memset(p, 0, sizeof(CrashCheckpoint));
    p->magic   = MAGIC; // 'CRCH'
    p->version = VERSION;
    strcpy(p->crash_reason, "init");
}

void CrashCheckpoint_init() {
    if (! file_system_is_running()) {	// If FS fails, we still run, just no crash logging.
        return;
    }
    // Try to read existing checkpoint
    CrashCheckpoint ccp;
    File f = FileSys.open(checkpoint_filename, "r");
    if (!f) {
    	init_checkpoint_struct(&ccp);		// No previous checkpoint; initialize defaults
    	return;
    }
    if (f.read((uint8_t*)&ccp, sizeof(ccp)) != sizeof(ccp)) {
    	init_checkpoint_struct(&ccp);		// Corrupt or wrong size; reset
    }
    f.close();

    // Initialize heap low-water from last run to provide a multirun low water mark
	//    g_heap_low_water = ccp.heap_low_water;
	//    if (g_heap_low_water == 0 || g_heap_low_water == 0xFFFFFFFF) {
	//        g_heap_low_water = ESP.getFreeHeap();
	//    }
    // Initialize heap low-water just for this run
    g_heap_low_water = ESP.getFreeHeap();

    String reason = ESP.getResetReason();
    safestrncpy(ccp.crash_reason, reason.c_str(), 32);
    CrashLog_writeTmpLog(ccp);
    // can add more info as needed, e.g. heap, real time, app text/flags
    SPRNTF(1, "Last boot reason: %s, %u, %u\n", ccp.crash_reason, ccp.app_state_value, ccp.uptime_s);
}


static void CrashCheckpoint_update(uint32_t app_flags,
                            uint32_t app_value,
                            const char *app_text,
                            uint32_t rtc_unix_time /* 0 if not available */)
{
    // Update heap low-water
    uint32_t current_free = ESP.getFreeHeap();
    if (current_free < g_heap_low_water) {
        g_heap_low_water = current_free;
    }

    CrashCheckpoint ccp;
    // Fill checkpoint struct
    ccp.magic          = MAGIC;
    ccp.version        = VERSION;

    ccp.uptime_ms      = millis();
    ccp.uptime_s       = ccp.uptime_ms / 1000;

    ccp.heap_free      = current_free;
    ccp.heap_low_water = g_heap_low_water;

    ccp.app_state_flags = app_flags;
    ccp.app_state_value = app_value;

    ccp.rtc_time        = rtc_unix_time;

    // Copy app_text safely into fixed buffer
    if (app_text != nullptr) {
        size_t len = strlen(app_text);
        if (len >= sizeof(ccp.app_state_text)) {
            len = sizeof(ccp.app_state_text) - 1;
        }
        memcpy(ccp.app_state_text, app_text, len);
        ccp.app_state_text[len] = '\0';
    } else {
        ccp.app_state_text[0] = '\0';
    }
    strcpy(ccp.crash_reason, "none");

    // Write to flash (overwrite single file)
    File f = FileSys.open(checkpoint_filename, "w");
    if (!f) {
        return; // silently fail; nothing else we can do
    }
    f.write((const uint8_t*)&ccp, sizeof(ccp));
    f.close();
}


// to be called from a loop(), e.g. wifi::loop

static const uint32_t checkpoint_interval = 5000;
static uint32_t g_last_checkpoint_ms = 0;

void CrashCheckpoint_periodic(uint32_t app_flags,
                              uint32_t app_value,
                              const char *app_text,
                              uint32_t rtc_unix_time)
{
    uint32_t now = millis();
    if (now - g_last_checkpoint_ms >= checkpoint_interval) { // every 5 seconds
        g_last_checkpoint_ms = now;
        CrashCheckpoint_update(app_flags, app_value, app_text, rtc_unix_time);
    }
}




/*
bool CrashLog::load(CrashInfo &ci) {
    File f = FileSys.open("/crash-0.log", "r");
    if (!f) return false;

    String line = f.readStringUntil('\n');
    f.close();

    if (line.length() == 0) return false;

    int p1 = line.indexOf(',');
    int p2 = line.indexOf(',', p1 + 1);

    ci.reason    = line.substring(0, p1);
    ci.lastState = line.substring(p1 + 1, p2).toInt();
    ci.uptime    = line.substring(p2 + 1).toInt();

    return true;
}
*/
