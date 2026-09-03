/*
 * WebSocket.h
 *
 *  Created on: Aug. 20, 2026
 *      Author: Danny
 */

#ifndef WEBSOCKET_H_
#define WEBSOCKET_H_


#include "WebSocketBase.h"


class WebSocketApp : public WebSocketBase {
public:
    WebSocketApp(uint16_t port = 81);

protected:
    bool restoreDataFromFile() override;
    void saveDataToFile() override;
    void deleteDataFromFile() override;

    void handleAppWebSocketMessage(uint8_t num,
                                   WStype_t type,
                                   String &msg) override;

    void getTimeAdjustmentParameters(int &tz, int &dst, int &offst) override;	// supplied by app from UI

private:
    void sendInitData(uint8_t num);
    void do_activatemanual(uint8_t num);
    void do_activateauto(uint8_t num);
    void do_opendamper(uint8_t num);
    void do_closedamper(uint8_t num);

// helpers
    void setVarByName(char *varname, int varval, int radio);
    void showGVars();
};




#endif /* WEBSOCKET_H_ */
