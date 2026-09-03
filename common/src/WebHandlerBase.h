/*
 * webhandler.h
 *
 *  Created on: Sep. 23, 2019
 *      Author: Danny
 */

#ifndef WEBHANDLERBASE_H_
#define WEBHANDLERBASE_H_

#include <ESP8266WebServer.h>

#include "DataSaver.h"

class wifi;
class WebSocketBase;

class WebHandlerBase
{
public:
	void sendSettingsPage();

	WebHandlerBase(const char *txtnm);
	virtual ~WebHandlerBase() = default;

	void setupWebhandling(bool as_station);
	void loopWebhandling();

	bool restoreLibraryData();
	virtual bool restoreAppData() = 0;
	bool saveLibraryData();
	virtual bool saveAppData() = 0;

protected:
	ESP8266WebServer httpServer{80};

	// Extension points for derived classes
    virtual void registerAppRoutes() = 0;
    virtual void renderAppDebugInfo() = 0;
    virtual void handleAppSettings() = 0;

    // Optional WebSocket extension
    virtual void handleAppWebSocketMessage(const char *msg) { }

    // Shared helpers
    void registerCommonRoutes(bool as_station);
    void renderCommonDebugInfo();
    void handleCommonSettings();
    void handleFileUpload();

    void handleDebugPage();      // parcels out responsibilities between base & app

    // WebSocket plumbing
    void wsSend(const char *msg);

    //const char *genericName; - replaced by static textname below

private:
    // Helper functions (that reference httpServer)
    bool handleFileRead(String path);
    void handleNotFound(String path);
    void handleIgnore();
    void handleGeneric();
    void debugSetLoopDelay();
    void debugGetFile();
    void debugPutFile();
    void saveSettings();			// may move up?


//------------------
    static const char *textname;			// static because debugSendPage is static, because it's a callback
	static const wifi *wifi_instance;
	WebSocketBase *webSocketHandler;
	DataSaver *libraryDataSaver;
	struct {
		int dt;
	} libraryData;


public:
	void setWifi(wifi *thewifi);
	void setSocketHandler(WebSocketBase *sockethandler);
	void debugSendPage();
	//void setup_http(bool as_station);

};


#endif /* WEBHANDLER_H_ */
