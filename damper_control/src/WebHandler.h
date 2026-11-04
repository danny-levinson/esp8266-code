/*
 * WebHandler.h
 *
 *  Created on: Aug. 18, 2026
 *      Author: Danny
 */

#ifndef WEBHANDLER_H_
#define WEBHANDLER_H_

    // Optional WebSocket extension
    // virtual void handleAppWebSocketMessage(const String &msg) { }


#include "WebHandlerBase.h"


class WebHandler : public WebHandlerBase
{
public:
	WebHandler(const char *genericname);

	bool restoreAppData() override;
	bool saveAppData() override;

	const char *getAppVersion() override;
    void registerAppRoutes(bool asStation) override;
	void renderAppDebugInfo() override;
	void appLoopWebhandling() override;

private:
	DataSaver *appDataSaver;
	struct {
		int ang1, ang2;
	} appData;

	void debug_set_servo_angles();
};


#endif /* WEBHANDLER_H_ */
