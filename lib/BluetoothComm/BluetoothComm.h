#ifndef BLUETOOTH_COMM_H
#define BLUETOOTH_COMM_H

#include <Arduino.h>
#include <WiFi.h>
extern "C" {
  #include "esp_wifi.h"
  #include "tcpip_adapter.h"
}

#ifdef CHAIR_MASTER
#include <HTTPClient.h>
#endif

#ifdef CHAIR_SLAVE
#include <ESPAsyncWebServer.h>
#endif

class BluetoothComm {
public:
    BluetoothComm();
    
#ifdef CHAIR_MASTER
    // Master chair functions
    void beginMaster(const String& deviceName = "RoboticChairMaster");
    void sendCommand(int degrees, const String& direction, int speed);
    bool isSlaveConnected();
    void checkConnection();
    String findSlaveIP();
#endif

#ifdef CHAIR_SLAVE
    // Slave chair functions  
    void beginSlave(const String& deviceName = "RoboticChairSlave");
    bool checkForCommand();
    String getLastCommand();
    void parseCommand(const String& command, int& steps, float& stepDelay);
#endif

private:
#ifdef CHAIR_MASTER
    HTTPClient httpClient;
    bool slaveConnected = false;
    unsigned long lastConnectionCheck = 0;
    const unsigned long CONNECTION_CHECK_INTERVAL = 5000; // 5 seconds
    
    // Hardcoded slave MAC address - replace with your actual slave MAC
    const String SLAVE_MAC_ADDRESS = "24:0A:C4:XX:XX:XX"; // TODO: Replace XX:XX:XX with actual slave MAC
    
    // Helper methods
    String macToString(const uint8_t* mac);
    bool isSlaveMAC(const String& mac);
    String getIPForMAC(const String& targetMAC);
    void setupWiFiEvents();
    static void onStationConnected(WiFiEvent_t event, WiFiEventInfo_t info);
    static void onStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);
#endif

#ifdef CHAIR_SLAVE
    AsyncWebServer server{80};
    volatile int receivedSteps = 0;
    volatile float receivedDelay = 0;
    volatile bool newCommandReceived = false;
#endif
};

// Global instance
extern BluetoothComm btComm;

#endif // BLUETOOTH_COMM_H
