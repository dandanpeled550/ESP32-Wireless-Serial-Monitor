#ifndef WIRELESS_MONITOR_H
#define WIRELESS_MONITOR_H

#ifdef CHAIR_MASTER
#include <Arduino.h>
#include <AsyncTCP.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <WebSocketsServer.h>
#include <WiFi.h>

// WirelessMonitor Class
class WirelessMonitor {
 public:
  WirelessMonitor();
  void print(const String &message);
  void setup();
  void loop();

 private:
  DNSServer dnsServer;
  AsyncWebServer server;
  WebSocketsServer webSocket;
  String logBuffer;
  void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t *payload,
                        size_t length);
  void initWiFi();
  void setupServer();
  void initDNS();
  void initmDNS();
};

// CaptivePortalHandler Class
class CaptivePortalHandler : public AsyncWebHandler {
 public:
  CaptivePortalHandler();
  virtual ~CaptivePortalHandler();

  bool canHandle(AsyncWebServerRequest *request);
  void handleRequest(AsyncWebServerRequest *request);
};

// Global instance declaration (define exactly once in WirelessMonitor.cpp)
extern WirelessMonitor wm;

#endif  // CHAIR_MASTER
#endif  // WIRELESS_MONITOR_H