#ifndef WiFiVirtuino_h
#define WiFiVirtuino_h

#include <WiFi.h>
#include "Virtuino_ESP_WifiServer.h"
//#include "TimeRTCaGPS.h"
#include "RTClib.h"
#define BUTTON_WIFI 12

class WiFiVirtuino
{
public:
    void setupVirtuino();
    void loopVirtuino(DateTime datetime);
    void  connectToWiFiNetwork();
    void initAccessPoint();
    void WiFiSetup();
    float value1;
    void WriteInt(uint8_t VirtuinoRegister, int VirtuinoValue);
    String DisplayIPAddress();
    int variable();
    int Switch();
    int variablewrite();

private:
};

#endif
