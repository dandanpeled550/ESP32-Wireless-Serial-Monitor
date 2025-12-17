/**
   (`\ .-') /`       _  .-')     ('-.              ('-.    .-')     .-') .-')
('-.  _  .-')             ('-.
   `.( OO ),'      ( \( -O )  _(  OO)           _(  OO)  ( OO ).  ( OO ). ( OO
).  _(  OO)( \( -O )           ( OO ).-.
,--./  .--.  ,-.-') ,------. (,------.,--.     (,------.(_)---\_)(_)---\_)
(_)---\_)(,------.,------.  ,-.-')   / . --. / ,--. |      |  |  |  |OO)|   /`.
' |  .---'|  |.-')  |  .---'/    _ | /    _ |       /    _ |  |  .---'|   /`. '
|  |OO)  | \-.  \  |  |.-') |  |   |  |, |  |  \|  /  | | |  |    |  | OO ) |  |
\  :` `. \  :` `.       \  :` `.  |  |    |  /  | | |  |  \.-'-'  |  | |  | OO )
|  |.'.|  |_)|  |(_/|  |_.' |(|  '--. |  |`-' |(|  '--.  '..`''.) '..`''.)
'..`''.)(|  '--. |  |_.' | |  |(_/ \| |_.'  | |  |`-' | |         | ,|  |_.'|  .
'.' |  .--'(|  '---.' |  .--' .-._)   \.-._)   \      .-._)   \ |  .--' |  .
'.',|  |_.'  |  .-.  |(|  '---.' |   ,'.   |(_|  |   |  |\  \  |  `---.|      |
|  `---.\       /\       /      \       / |  `---.|  |\  \(_|  |     |  | |  | |
|
'--'   '--'  `--'   `--' '--' `------'`------'  `------' `-----'  `-----'
`-----'  `------'`--' '--' `--'     `--' `--' `------'
*/

// Headers
#include "WirelessMonitor.h"

#include "EmbeddedFiles.h"
#include "StepperMover.h"

// Single global instance
WirelessMonitor wm;


// Constants
// the adderess serial monitor is available at http://serialmonitor.local -- can
// be changed in 'embeddedfiles.cpp'
const char *ssid = "ESP32-AP";
const char *password = "12345678";
const byte DNS_PORT = 53;
const int ws_port = 81;
const int server_port = 80;
const int serial_port = 9600;

// Wireless Monitor Class Implementation
WirelessMonitor::WirelessMonitor()
    : webSocket(ws_port), logBuffer(""), server(server_port) {}

void WirelessMonitor::print(const String &message) {
  Serial.println(message);
  logBuffer += message + "<br>";
  webSocket.broadcastTXT(message.c_str());
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
    webSocket.sendTXT(num, logBuffer);
  }
}

void WirelessMonitor::initWiFi() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
}

void WirelessMonitor::setupServer() {
  server.on("/", HTTP_GET, serveIndexHtml);          // Serve the embedded HTML
  server.on("/style.css", HTTP_GET, serveStyleCss);  // Serve the embedded CSS
  server.on("/script.js", HTTP_GET, serveScriptJs);  // Serve the embedded JS
  server.on("/rotate", HTTP_GET, [](AsyncWebServerRequest *request) {
    int degrees = 90;  // default degrees
    String direction = "clockwise";  // default direction
    int speedLevel = 5;  // default (1-10 scale)

    if (request->hasParam("degrees")) {
        degrees = abs(request->getParam("degrees")->value().toInt()); // Always positive
    }
    
    if (request->hasParam("direction")) {
        direction = request->getParam("direction")->value();
    }
    
    if (request->hasParam("speed")) {
        speedLevel = request->getParam("speed")->value().toInt();
        // Clamp to valid range
        if (speedLevel < 1) speedLevel = 1;
        if (speedLevel > 10) speedLevel = 10;
    }

    // Convert degrees to steps: 450 steps = 90 degrees, so steps = degrees * 5
    int steps = degrees * 5;
    
    // Apply direction: clockwise = positive, counter-clockwise = negative
    if (direction == "counter-clockwise") {
        steps = -steps;
    }

    // Convert user speed (1-10) to milliseconds between steps
    // Speed 10: ~8.3ms per step (3x faster than before)
    // Speed 1:  ~100ms per step (slowest)
    // Formula: 108.3 - (speedLevel * 10) = range from 98.3ms to 8.3ms
    float stepDelay = 108.3 - (speedLevel * 10.0);
    Stepper.speed = stepDelay;

    Serial.printf("Rotate requested, degrees = %d %s (%d steps), speed = %d (%.1fms/step)\n", 
                  degrees, direction.c_str(), steps, speedLevel, stepDelay);
    stepperMove(steps);

    request->send(200, "text/plain", "OK"); 
  });

  server.addHandler(new CaptivePortalHandler()).setFilter(ON_AP_FILTER);

  server.onNotFound([&](AsyncWebServerRequest *request) {
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
  // Handle all requests
  return true;
}

void CaptivePortalHandler::handleRequest(AsyncWebServerRequest *request) {
  // Serve a simple HTML page
  request->send(200, "text/html", captive_html);
  
}
