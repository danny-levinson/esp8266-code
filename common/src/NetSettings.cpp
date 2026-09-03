/*
 * settings.cpp
 *
 *  Created on: Oct. 6, 2019
 *      Author: Danny
 */


#include "NetSettings.h"

#include "DataSaver.h"


NetSettings *net_settings = NULL;

const char *cfdname = "/netdatanew";

// could also use EEPROM for these settings

static struct {
	char pubname[MAXNAMELEN+1];
	char netssid[MAXNAMELEN+1];
	char netpasswd[MAXNAMELEN+1];
} settingsData;

static DataSaver *netSaver = 0;


NetSettings::NetSettings() {
	setup_file_system();
	print_files_to_serial(false);
	netSaver = new DataSaver(&settingsData, sizeof(settingsData), cfdname);
	settingsData.pubname[0] = settingsData.netssid[0] = settingsData.netpasswd[0] = '\0';
}

bool NetSettings::settingsExist() {
	return FileSys.exists(cfdname);
}

bool NetSettings::read(int maxcflen, char *publicname, char *ssid, char *password) {
	if (! netSaver->read()) return false;
	safestrncpy(publicname, settingsData.pubname, maxcflen);
	safestrncpy(ssid, settingsData.netssid, maxcflen);
	safestrncpy(password, settingsData.netpasswd, maxcflen);
	return true;
}

bool NetSettings::save(const char *pubname, const char *netname, const char *netpass) {
	safestrncpy(settingsData.pubname, pubname, MAXNAMELEN);
	safestrncpy(settingsData.netssid, netname, MAXNAMELEN);
	safestrncpy(settingsData.netpasswd, netpass, MAXNAMELEN);
	return netSaver->write();
}

void NetSettings::remove() {
	if (FileSys.exists(cfdname)) {
		FileSys.remove(cfdname);
	}
}
