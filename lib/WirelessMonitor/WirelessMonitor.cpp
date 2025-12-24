


#ifdef CHAIR_MASTER
// Headers
#include "WirelessMonitor.h"

#include "EmbeddedFiles.h"
#include "StepperMover.h"
#include "BluetoothComm.h"

// Single global instance
WirelessMonitor wm;

// Constants
// the adderess serial monitor is available at http://serialmonitor.local -- can
// be changed in 'embeddedfiles.cpp'
const byte DNS_PORT = 53;
const int ws_port = 81;
const int server_port = 80;
const int serial_port = 9600;
// WiFi credentials (global, visible to both master and slave)
const char *ssid = "RoboticBarStools";
const char *password = "milabspirit";


// Wireless Monitor Class Implementation
WirelessMonitor::WirelessMonitor()
    : webSocket(ws_port), logBuffer(""), server(server_port) {}

void WirelessMonitor::print(const String &message) {
  Serial.println(message);
  logBuffer += message + "<br>";
  if (webSocket.connectedClients() > 0) {
    webSocket.broadcastTXT(message.c_str());
  }
}
void WirelessMonitor::setup() {
  initWiFi();
  setupServer();
    Serial.println("Test started");
  initDNS(); //responsible for resolving the domain name to the IP address 
  server.begin(); //responsible for serving the index.html file to clients
  Serial.println("Server started");
  initmDNS(); //responsible for resolving the domain name to the IP address 
  Serial.println("mDNS responder started");
  webSocket.begin(); //responsible for sending Logs to clients
  webSocket.onEvent(
      [this](uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
        this->onWebSocketEvent(num, type, payload, length);
      });

  Serial.println("WebSocket server started");
  Serial.println("Wireless Monitor setup complete\n");
}
void WirelessMonitor::loop() {
  dnsServer.processNextRequest();
  webSocket.loop();
}

void WirelessMonitor::onWebSocketEvent(uint8_t num, WStype_t type,
                                       uint8_t *payload, size_t length) {
  if (type == WStype_TEXT) {
    Serial.printf("WebSocket[%u] received: %s\n", num, payload);
  } else if (type == WStype_CONNECTED) {
    Serial.printf("New web connection established - Client[%u] connected\n", num);
    webSocket.sendTXT(num, logBuffer);
  } else if (type == WStype_DISCONNECTED) {
    Serial.printf("Web connection closed - Client[%u] disconnected\n", num);
  }
}

void WirelessMonitor::initWiFi() {
  WiFi.mode(WIFI_AP);
  bool result = WiFi.softAP(ssid, password);
  
  if (result) {
    Serial.println("WiFi AP started successfully!");
    Serial.printf("SSID: %s\n", ssid);
    Serial.printf("Password: %s\n", password);
    Serial.printf("AP IP: %s\n", WiFi.softAPIP().toString().c_str());
  } else {
    Serial.println("Failed to start WiFi AP!");
  }
}

void WirelessMonitor::setupServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("HTTP GET request for root page received");
    Serial.flush(); // Ensure serial output is flushed before HTTP response
    request->send_P(200, "text/html", captive_html);
    Serial.println("Sent captive HTML with chair controls");
  });
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("HTTP GET request for CSS received");
    serveStyleCss(request);
  });
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("HTTP GET request for JS received");
    serveScriptJs(request);
  });
  server.on("/monitor", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("HTTP GET request for serial monitor received");
    serveIndexHtml(request);
  });
 server.on("/hello", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("HTTP GET request for /hello received");
    if (!request->hasParam("mac") || !request->hasParam("ip")) {
        request->send(400, "text/plain", "missing mac or ip");
        return;
    }

    String mac = request->getParam("mac")->value();
    mac.toUpperCase();

    IPAddress ip;
    if (!ip.fromString(request->getParam("ip")->value())) {
        request->send(400, "text/plain", "invalid ip");
        return;
    }
    
    Serial.printf("✓ HELLO from %s at %s\n",
                  mac.c_str(),
                  ip.toString().c_str());
    Serial.println("IP Address received from slave chair." + ip.toString());
    request->send(200, "text/plain", "hello registered");
    btComm.cachedSlaveIP = ip.toString();
  });
  server.on("/rotate", HTTP_GET, [](AsyncWebServerRequest *request) {
    static uint32_t rotateReqId = 0;
    rotateReqId++;

    // Log once per HTTP request (useful to detect duplicates)
    String remoteIp = request->client() ? request->client()->remoteIP().toString() : String("?");
    Serial.printf("[ROTATE HTTP] id=%lu from=%s url=%s\n",
                  (unsigned long)rotateReqId,
                  remoteIp.c_str(),
                  request->url().c_str());

    int degrees = 90;               // default degrees
    String direction = "clockwise"; // default direction
    int speedLevel = 5;             // default (1-10 scale)
    String chair = "master";        // default chair

    if (request->hasParam("degrees")) {
        degrees = abs(request->getParam("degrees")->value().toInt()); // Always positive
    }

    if (request->hasParam("direction")) {
        direction = request->getParam("direction")->value();
    }

    if (request->hasParam("speed")) {
        speedLevel = request->getParam("speed")->value().toInt();
        if (speedLevel < 1) speedLevel = 1;
        if (speedLevel > 10) speedLevel = 10;
    }

    if (request->hasParam("chair")) {
        chair = request->getParam("chair")->value();
    }

    // Convert degrees to steps: 450 steps = 90 degrees, so steps = degrees * 5
    int steps = degrees * 5;

    // Apply direction: clockwise = positive, counter-clockwise = negative
    if (direction == "counter-clockwise") {
        steps = -steps;
    }

    // Convert user speed (1-10) to milliseconds between steps
    float stepDelay = 108.3f - (speedLevel * 10.0f);

    Serial.printf("Rotate requested: chair=%s, degrees=%d, dir=%s, steps=%d, speed=%d (%.1fms/step)\n",
                  chair.c_str(), degrees, direction.c_str(), steps, speedLevel, stepDelay);

    // IMPORTANT:
    // 1) Always send an HTTP response exactly once.
    // 2) If chair == both, execute BOTH paths (master + slave).

    bool didMaster = false;
    bool didSlave = false;
    bool slaveOk = true;

    // Execute on master chair (this chair)
    if (chair == "master" || chair == "both") {
        didMaster = true;
        Serial.println("Executing command on master chair locally");

        Stepper.speed = stepDelay;
        stepperMove(steps);

        Serial.println("Master chair command executed locally");
    }

    // Execute on slave chair (if requested)
    if (chair == "slave" || chair == "both") {
        didSlave = true;
        Serial.println("Preparing to send command to slave chair");

        if (btComm.isSlaveConnected()) {
            btComm.sendCommand(steps, stepDelay);
            Serial.printf("Command sent to slave chair at %s\n", btComm.cachedSlaveIP.c_str());
        } else {
            slaveOk = false;
            Serial.println("No slave chair found - command not sent");
        }
    }

    // Build a single response
    String resp = "OK ";
    if (didMaster) resp += "master ";
    if (didSlave) resp += "slave ";
    resp += "| degrees=" + String(degrees);
    resp += " dir=" + direction;
    resp += " speed=" + String(speedLevel);

    if (didSlave && !slaveOk) {
        request->send(500, "text/plain", "Error: No slave chair connected.");
        return;
    }

    request->send(200, "text/plain", resp);
  });

  // Slave status endpoint for web interface (lightweight)
  server.on("/slave-status", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Use cached IP only - don't trigger expensive scanning
    bool isConnected = btComm.isSlaveConnected();
    
    String jsonResponse = "{";
    jsonResponse += "\"connected\":" + String(isConnected ? "true" : "false");
    if (isConnected) {
      jsonResponse += ",\"ip\":\"" + btComm.cachedSlaveIP + "\"";
    }
    jsonResponse += "}";
    
    request->send(200, "application/json", jsonResponse);
    // No debug output to avoid spam
  });

  // New endpoint to trigger reconnection to slave
  server.on("/trigger-reconnect", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("User-triggered slave reconnection attempt.");

    String slaveIP = btComm.findSlaveIP();
    if (!slaveIP.isEmpty()) {
        btComm.cachedSlaveIP = slaveIP;
        Serial.println("Slave reconnected successfully at: " + slaveIP);
        request->send(200, "text/plain", "Slave reconnected successfully at: " + slaveIP);
    } else {
        Serial.println("Failed to reconnect to slave.");
        request->send(500, "text/plain", "Error: Failed to reconnect to slave.");
    }
});

  //TODO: add an option to triger slave connection from web interface?
  server.addHandler(new CaptivePortalHandler()).setFilter(ON_AP_FILTER);

  server.onNotFound([&](AsyncWebServerRequest *request) {
    Serial.printf("HTTP request for unknown page: %s\n", request->url().c_str());
    request->send(200, "text/html", captive_html);
  });
}

void WirelessMonitor::initDNS() {
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.setTTL(300);
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
}

void WirelessMonitor::initmDNS() {
  if (!MDNS.begin("serialmonitor")) {
    Serial.println("Error setting up MDNS responder!");
  }
}

// CaptivePortalHandler Class Implementation
CaptivePortalHandler::CaptivePortalHandler() {}

CaptivePortalHandler::~CaptivePortalHandler() {}

bool CaptivePortalHandler::canHandle(AsyncWebServerRequest *request) {
  String url = request->url();
  if (url == "/rotate" ||
      url == "/slave-status" ||
      url == "/trigger-reconnect" ||
      url == "/style.css" ||
      url == "/script.js" ||
      url == "/monitor") {
    return false;
  }
  return true;
}

void CaptivePortalHandler::handleRequest(AsyncWebServerRequest *request) {
  // Serve a simple HTML page
  request->send(200, "text/html", captive_html);
  
}
#endif  // CHAIR_MASTER
