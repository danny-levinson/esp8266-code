/*
 * WebHandler.h
 *
 *  Created on: Sep. 29, 2026
 *      Author: Danny
 */

#ifndef WEBHANDLER_H_
#define WEBHANDLER_H_

#include <CRC32.h>


#include "WebHandlerBase.h"

    // Optional WebSocket extension
    // virtual void handleAppWebSocketMessage(const String &msg) { }

// used to communicate from main to SSDP initialisation
class ServiceIdentity
{
public:
	ServiceIdentity() : serviceType(0), serviceLocation(0) {}
	void setIdentity(const char *st, const char *sl) {
		serviceType = st;
	 	serviceLocation = sl;
	}

	const char *getType() {return serviceType; }
	const char *getLocation() { return serviceLocation; }
private:
	const char *serviceType;			// identifies the class of the device
	const char *serviceLocation;		// uniquely identifies the device
};

class WebHandler : public WebHandlerBase
{
public:
	WebHandler(const char *genericname);

	void setIdentity(const char *st, const char *sl) {
		ident.setIdentity(st, sl);
	}

	bool restoreAppData() override;
	bool saveAppData() override;

	const char *getAppVersion() override;
    void registerAppRoutes(bool asStation) override;
	void renderAppDebugInfo() override;
	void appLoopWebhandling() override;

private:
	ServiceIdentity ident;
	void setupSSDP();

	DataSaver *appDataSaver;
	struct {
		int calib;
	} appData;

	File sendingFile;
	WiFiClient sendingClient;
	bool sendingFileSet;
	int  sendingFileTime;
	CRC32 sendingChecksum;

	void temperaturesSendPage();
	void configSendPage();
	void configSaveData();

	void syncBuffer();
	void listFilesToClient();
	void getRawFile();
	void getRawFileChecksum();
	bool sendRawFile(const char *path);
	void sendNextFileChunk();
	void send_octets_preamble(WiFiClient &client, unsigned int len);
};


#endif /* WEBHANDLER_H_ */
