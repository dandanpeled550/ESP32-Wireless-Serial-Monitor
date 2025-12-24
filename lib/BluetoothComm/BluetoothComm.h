#ifndef BLUETOOTH_COMM_H
#define BLUETOOTH_COMM_H

#include <Arduino.h>
#include <WiFi.h>
extern "C" {
  #include "esp_wifi.h"
  #include "tcpip_adapter.h"
}


#include <HTTPClient.h>

#ifdef CHAIR_SLAVE
#include <ESPAsyncWebServer.h>
#endif

class BluetoothComm {
public:
    BluetoothComm();
    

#ifdef CHAIR_MASTER
    // Master chair functions
    void beginMaster(const String& deviceName = "RoboticChairMaster");
    void sendCommand(int steps, float stepDelay);
    bool isSlaveConnected();
    void checkConnection();
    String findSlaveIP();

    // Public access to cached IP for status checks
    String cachedSlaveIP = ""; // Cache the slave IP to avoid repeated scans

    // Hardcoded slave MAC address - replace with your actual slave MAC
    const String SLAVE_MAC_ADDRESS = "c4:4f:33:08:1a:4d"; // TODO: Replace XX:XX:XX with actual slave MAC
#endif

#ifdef CHAIR_SLAVE
    // Slave chair functions  
    void beginSlave(const String& deviceName = "RoboticChairSlave");
    bool checkForCommand();
    String getLastCommand();
    void parseCommand(const String& command, int& steps, float& stepDelay);
    void maintainConnection(); // Call this in main loop
    void maintainMasterConnection(); // Call this in main loop to trigger slave announcement
    void announceToMaster(); // Actually send the POST to master
    // Connection status to master
    bool isConnectedToMaster = false;
#endif

private:
#ifdef CHAIR_MASTER
    HTTPClient httpClient;
    bool slaveConnected = false;
    unsigned long lastConnectionCheck = 0;
    const unsigned long CONNECTION_CHECK_INTERVAL = 5000; // 5 seconds

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
    
    // Reconnection mechanism
    unsigned long lastReconnectAttempt = 0;
    const unsigned long RECONNECT_INTERVAL = 5000; // Try every 5 seconds
    bool serverStarted = false;
    void attemptReconnection();
#endif
};

// Global instance
extern BluetoothComm btComm;

#endif // BLUETOOTH_COMM_H
