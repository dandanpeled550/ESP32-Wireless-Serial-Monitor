#include "BluetoothComm.h"

// Global instance
BluetoothComm btComm;

BluetoothComm::BluetoothComm() {}

#ifdef CHAIR_MASTER
void BluetoothComm::beginMaster(const String& deviceName) {
    Serial.println("WiFi Master started: " + deviceName);
    Serial.println("Use web interface to rotate this chair and connected slave chairs");
    Serial.println("Slaves can connect to WiFi and receive commands via HTTP");
    
    // Initialize HTTP client for sending commands to slaves
    httpClient.setTimeout(1000);  // 1 second timeout
    Serial.println("HTTP client ready to send commands to slave chairs");
}



void BluetoothComm::sendCommand(int degrees, const String& direction, int speed) {
    // Send command to slave chair via WiFi HTTP request
    String slaveIP = findSlaveIP();
    
    if (slaveIP.isEmpty()) {
        Serial.println("No slave chair found on WiFi network");
        return;
    }
    
    // Pre-process on master side
    int steps = degrees * 5;  // Convert degrees to steps
    if (direction == "counter-clockwise") {
        steps = -steps;
    }
    
    // Convert speed to stepDelay
    float stepDelay = 108.3 - (speed * 10.0);
    
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
    // Simple approach: try common IP addresses on the network
    // In a real implementation, you might use mDNS or DHCP lease table
    IPAddress baseIP = WiFi.localIP();
    String baseIPStr = String(baseIP[0]) + "." + String(baseIP[1]) + "." + String(baseIP[2]) + ".";
    
    // Try scanning common IP addresses (this is a simple approach)
    for (int i = 100; i <= 120; i++) {
        String testIP = baseIPStr + String(i);
        if (testIP != WiFi.localIP().toString()) {
            // Quick ping test by attempting HTTP connection
            httpClient.begin("http://" + testIP + "/execute");
            httpClient.setTimeout(200);  // Very short timeout for ping
            int code = httpClient.GET();
            httpClient.end();
            
            if (code == 400 || code == 200) {  // 400 = missing params, but server responded
                Serial.println("Found slave chair at IP: " + testIP);
                return testIP;
            }
        }
    }
    
    return "";  // No slave found
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
#endif

#ifdef CHAIR_SLAVE
void BluetoothComm::beginSlave(const String& deviceName) {
    // Connect to master's WiFi instead of using Bluetooth
    WiFi.mode(WIFI_STA);
    Serial.println("WiFi Slave started");
    Serial.println("Connecting to master's WiFi: RoboticBarStools");
    
    WiFi.begin("RoboticBarStools", "12345678");
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("");
        Serial.println("Connected to master's WiFi!");
        Serial.print("Slave IP address: ");
        Serial.println(WiFi.localIP());
        
        // Setup HTTP server to receive commands
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
        
        server.begin();
        Serial.println("Slave HTTP server started on /execute");
    } else {
        Serial.println("Failed to connect to master's WiFi!");
    }
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
#endif