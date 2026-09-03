/*
 * WebSocketBase.h
 *
 *  Created on: Aug. 20, 2026
 *      Author: Danny
 */

#ifndef WEBSOCKETBASE_H_
#define WEBSOCKETBASE_H_


#include <WebSocketsServer.h>


class WebSocketBase {
public:
    WebSocketBase(uint16_t port = 81);
    virtual ~WebSocketBase() = default;

    void setupWebsocket();
    void loopWebsocket();

    virtual bool restoreDataFromFile() = 0;		// state data
    virtual void saveDataToFile() = 0;
    virtual void deleteDataFromFile() = 0;

protected:
    virtual void getTimeAdjustmentParameters(int &tz, int &dst, int &offst) = 0;	// supplied by app from UI

    WebSocketsServer webSocket;

    // Extension point for app-specific WS messages
    virtual void handleAppWebSocketMessage(uint8_t num,
                                           WStype_t type,
                                           String &msg) = 0;

    // Common handlers
    void handleCommonWebSocketMessage(uint8_t num,
                                      WStype_t type,
                                      const String &msg);

    void wsSend(uint8_t num, const String &msg);
    void wsSend(uint8_t num, const char *msg, int len);

private:
    void sendTimeData(uint8_t num);
};



#endif /* WEBSOCKETBASE_H_ */
