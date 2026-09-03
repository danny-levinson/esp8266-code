/*
 * settings.h
 *
 *  Created on: Oct. 6, 2019
 *      Author: Danny
 */

#ifndef NETSETTINGS_H_
#define NETSETTINGS_H_

class NetSettings {
public:
	NetSettings();
	bool settingsExist();
	bool read(int maxcflen, char *publicname, char *ssid, char *password);
	bool save(const char *pubname, const char *netname, const char *netpass);
	void remove();
};

#define MAXNAMELEN 64

extern NetSettings *net_settings;

#endif /* NETSETTINGS_H_ */
