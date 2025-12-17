#ifndef BLUETOOTH_COMM_H
#define BLUETOOTH_COMM_H

#include <Arduino.h>
#include <WiFi.h>

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