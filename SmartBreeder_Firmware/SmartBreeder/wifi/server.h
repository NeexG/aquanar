#ifndef WIFI_SERVER_H
#define WIFI_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "config/config.h"

// Forward declarations
class PHSensor;
class TempSensor;
class FanControl;
class PHControl;

class SmartBreederServer {
private:
  WebServer* server;
  PHSensor* phSensor;
  TempSensor* tempSensor;
  FanControl* fanControl;
  PHControl* phControl;
  
  void handleRoot();
  void handleAPIStatus();
  void handleAPIControl();
  void handleAPISpecies();
  void handleAPICalibrate();
  void handleAPIWiFi();
  void handleAPIPing();
  void handleOptions();
  void setCORSHeaders();
  
  String getDashboardHTML();
  String getStatusJSON();
  
public:
  SmartBreederServer(PHSensor* ph, TempSensor* temp, FanControl* fan, PHControl* phCtrl);
  void begin();
  void update();
  bool isConnected();
  String getIP();
};

#endif

