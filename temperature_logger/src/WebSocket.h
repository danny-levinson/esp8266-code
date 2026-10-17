/*
 * WebSocket.h
 *
 *  Created on: Sep. 29, 2026
 *      Author: Danny
 */

#ifndef WEBSOCKET_H_
#define WEBSOCKET_H_


#include "WebSocketBase.h"


class WebSocketApp : public WebSocketBase {
public:
    WebSocketApp(uint16_t port = 81);

protected:
    void handleAppWebSocketMessage(uint8_t num,
                                   WStype_t type,
                                   String &msg) override;

private:
    void sendInitData(uint8_t num);

// helpers
    void setVarByName(char *varname, int varval, int radio);
};




#endif /* WEBSOCKET_H_ */
