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

protected:
    WebSocketsServer webSocket;

    // Extension point for app-specific WS messages
    virtual void handleAppWebSocketMessage(uint8_t num,
                                           WStype_t type,
                                           String &msg) = 0;

    // Common handlers
    bool handleCommonWebSocketMessage(uint8_t num,
                                      WStype_t type,
                                      const String &msg);

    void sendInitData(uint8_t num);

    void wsSend(uint8_t num, const String &msg);
    void wsSend(uint8_t num, const char *msg, int len);

    //Utilities
    char *getStrSocketPayloadParam(const char *payload, char *param, char *outstr, int outlen);
    void parseSocketChangePayload(const char *payload, char **varname, int &varval, int &radio);

private:
    void sendTimeData(uint8_t num);
};



#endif /* WEBSOCKETBASE_H_ */
