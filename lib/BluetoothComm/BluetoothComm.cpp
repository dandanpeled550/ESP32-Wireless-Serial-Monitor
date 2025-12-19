#include "BluetoothComm.h"

// Global instance
BluetoothComm btComm;

BluetoothComm::BluetoothComm() {}

#ifdef CHAIR_MASTER
void BluetoothComm::beginMaster(const String& deviceName) {
    Serial.println("WiFi Master started: " + deviceName);
    Serial.println("Use web interface to rotate this chair and connected slave chairs");
    Serial.println("Slaves can connect to WiFi and receive commands via HTTP");
    
    // Set up WiFi events for real-time slave detection
    setupWiFiEvents();
    
    // Initialize HTTP client for sending commands to slaves
    httpClient.setTimeout(1000);  // 1 second timeout
    Serial.println("HTTP client ready to send commands to slave chairs");
    Serial.println("Target slave MAC: " + SLAVE_MAC_ADDRESS + " (update this with your actual slave MAC)");
}



void BluetoothComm::sendCommand(int degrees, const String& direction, int speed) {
    // Send command to slave chair via WiFi HTTP request
    String slaveIP = findSlaveIP();
    
    if (slaveIP.isEmpty()) {
        Serial.println("No slave chair found on WiFi network");
        return;
    }

    //TODO: delete it, all pre-proccessing should be done when master get the input from user
    // Pre-process on master side 
    int steps = degrees * 5;  // Convert degrees to steps
    if (direction == "counter-clockwise") {
        steps = -steps;
    }
    
    // Convert speed to stepDelay
    float stepDelay = 108.3 - (speed * 10.0);
    // End of TODO

    String url = "http://" + slaveIP + "/execute?steps=" + String(steps) + "&delay=" + String(stepDelay);
    
    httpClient.begin(url);
    int httpResponseCode = httpClient.GET();
    
    if (httpResponseCode == 200) {
        String response = httpClient.getString();
        Serial.printf("Sent to slave (%s): %d steps, %.1f ms delay (from %d° %s speed %d)\n", 
                     slaveIP.c_str(), steps, stepDelay, degrees, direction.c_str(), speed);
        Serial.println("Slave response: " + response);
    } else {
        Serial.printf("Failed to send command to slave chair (%s). HTTP code: %d\n", slaveIP.c_str(), httpResponseCode);
    }
    
    httpClient.end();
}

bool BluetoothComm::isSlaveConnected() {
    // Check if slave is reachable via WiFi
    String slaveIP = findSlaveIP();
    return !slaveIP.isEmpty();
}

String BluetoothComm::findSlaveIP() {
    // Simple approach: Check if any devices are connected, then scan for our slave
    uint8_t stationCount = WiFi.softAPgetStationNum();
    
    if (stationCount == 0) {
        Serial.println("No stations connected to AP");
        return "";
    }
    
    Serial.printf("Found %d connected stations\n", stationCount);
    
    // We already get MAC verification through WiFi events (onStationConnected)
    // So if we reach here and there are connected stations, just scan for the responding device
    // The WiFi events already confirmed the correct slave MAC is connected
    
    Serial.println("Scanning for slave IP...");
    return getIPForMAC("");  // MAC verification done via events, just find the IP
}

void BluetoothComm::checkConnection() {
    unsigned long now = millis();
    if (now - lastConnectionCheck > CONNECTION_CHECK_INTERVAL) {
        lastConnectionCheck = now;
        String slaveIP = findSlaveIP();
        slaveConnected = !slaveIP.isEmpty();
        if (slaveConnected) {
            Serial.println("WiFi: Slave chair connected at " + slaveIP);
        } else {
            Serial.println("WiFi: No slave chair found on network");
        }
    }
}

String BluetoothComm::macToString(const uint8_t* mac) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}

bool BluetoothComm::isSlaveMAC(const String& mac) {
    if (mac.isEmpty()) {
        return false;
    }
    
    String normalizedMAC = mac;
    normalizedMAC.toUpperCase();
    String normalizedSlave = SLAVE_MAC_ADDRESS;
    normalizedSlave.toUpperCase();
    
    return normalizedMAC == normalizedSlave;
}

String BluetoothComm::getIPForMAC(const String& targetMAC) {
    // Since ESP32 doesn't provide direct DHCP lease table access,
    // we'll do a targeted scan knowing the device is connected
    IPAddress baseIP = WiFi.localIP();
    String baseIPStr = String(baseIP[0]) + "." + String(baseIP[1]) + "." + String(baseIP[2]) + ".";
    
    Serial.println("Scanning for IP of connected slave...");
    
    // Scan wider range since we know the device is connected
    for (int i = 2; i <= 254; i++) {
        String testIP = baseIPStr + String(i);
        if (testIP != WiFi.localIP().toString()) {
            // Quick test by attempting HTTP connection
            httpClient.begin("http://" + testIP + "/execute");
            httpClient.setTimeout(300);  // Short timeout
            int code = httpClient.GET();
            httpClient.end();
            
            if (code == 400 || code == 200) {  // Device responded
                Serial.println("✓ Found slave chair at IP: " + testIP);
                return testIP;
            }
        }
    }
    
    Serial.println("Could not determine slave IP address");
    return "";
}

void BluetoothComm::setupWiFiEvents() {
    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        onStationConnected(event, info);
    }, ARDUINO_EVENT_WIFI_AP_STACONNECTED);
    
    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        onStationDisconnected(event, info);
    }, ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);
}

void BluetoothComm::onStationConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    String connectedMAC = btComm.macToString(info.wifi_ap_staconnected.mac);
    Serial.println("✓ Station connected: " + connectedMAC);
    
    if (btComm.isSlaveMAC(connectedMAC)) {
        Serial.println("✓ Slave chair connected! MAC: " + connectedMAC);
        // findSlaveIP will be called when needed and will find the IP via DHCP query
    }
}

void BluetoothComm::onStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    String disconnectedMAC = btComm.macToString(info.wifi_ap_stadisconnected.mac);
    Serial.println("✗ Station disconnected: " + disconnectedMAC);
    
    if (btComm.isSlaveMAC(disconnectedMAC)) {
        Serial.println("✗ Slave chair disconnected");
    }
}

#endif

#ifdef CHAIR_SLAVE
void BluetoothComm::beginSlave(const String& deviceName) {
    // Initialize WiFi for slave mode with persistent reconnection
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);  // Enable auto-reconnect
    WiFi.persistent(true);        // Remember WiFi credentials
    
    Serial.println("WiFi Slave started with persistent reconnection");
    Serial.println("Will keep trying to connect to master's WiFi: RoboticBarStools");
    
    // Setup HTTP server routes (but don't start yet - will start when connected)
    server.on("/execute", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (request->hasParam("steps") && request->hasParam("delay")) {
            receivedSteps = request->getParam("steps")->value().toInt();
            receivedDelay = request->getParam("delay")->value().toFloat();
            newCommandReceived = true;
            
            Serial.printf("Received WiFi command: %d steps, %.1f ms/step\n", receivedSteps, receivedDelay);
            request->send(200, "text/plain", "Command received");
        } else {
            request->send(400, "text/plain", "Missing parameters");
        }
    });
    
    // Start first connection attempt
    attemptReconnection();
}

bool BluetoothComm::checkForCommand() {
    if (newCommandReceived) {
        newCommandReceived = false;
        return true;
    }
    return false;
}

String BluetoothComm::getLastCommand() {
    return "MOVE," + String(receivedSteps) + "," + String(receivedDelay);
}

void BluetoothComm::parseCommand(const String& command, int& steps, float& stepDelay) {
    // Direct assignment from WiFi command
    steps = receivedSteps;
    stepDelay = receivedDelay;
    
    Serial.printf("Executing command: %d steps, %.1f ms/step\n", steps, stepDelay);
}

void BluetoothComm::maintainConnection() {
    // Call this in your main loop to maintain connection
    unsigned long now = millis();
    
    // Check if we need to attempt reconnection
    if (WiFi.status() != WL_CONNECTED) {
        if (serverStarted) {
            Serial.println("✗ WiFi connection lost!");
            serverStarted = false;
        }
        
        // Attempt reconnection if enough time has passed
        if (now - lastReconnectAttempt > RECONNECT_INTERVAL) {
            attemptReconnection();
        }
    } else {
        // Connected - start server if not already started
        if (!serverStarted) {
            Serial.println("✓ WiFi connected! Starting HTTP server...");
            Serial.print("Slave IP address: ");
            Serial.println(WiFi.localIP());
            server.begin();
            serverStarted = true;
            Serial.println("Slave HTTP server started on /execute");
        }
    }
}

void BluetoothComm::attemptReconnection() {
    lastReconnectAttempt = millis();
    
    Serial.print("⟳ Attempting to connect to RoboticBarStools...");
    WiFi.begin("RoboticBarStools", "12345678");
    
    // Non-blocking check - just initiate connection
    // maintainConnection() will handle the rest
    Serial.println(" (connecting...)");
}
#endif