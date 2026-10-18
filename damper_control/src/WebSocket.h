/*
 * WebSocket.h
 *
 *  Created on: Aug. 20, 2026
 *      Author: Danny
 */

#ifndef WEBSOCKET_H_
#define WEBSOCKET_H_


#include "WebSocketBase.h"


class WebSocketApp : public WebSocketBase
{
public:
    WebSocketApp(uint16_t port = 81);

protected:

    void handleAppWebSocketMessage(uint8_t num,
                                   WStype_t type,
                                   String &msg) override;

private:
    void do_activatemanual(uint8_t num);
    void do_activateauto(uint8_t num);
    void do_opendamper(uint8_t num);
    void do_closedamper(uint8_t num);
};




#endif /* WEBSOCKET_H_ */
