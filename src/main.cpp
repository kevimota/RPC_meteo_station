/*********
  Rui Santos
  Complete instructions at https://RandomNerdTutorials.com/esp32-wi-fi-manager-asyncwebserver/

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*********/
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ESPmDNS.h>

#include "FS.h"
#include "LITTLEFS.h"

#ifndef CONFIG_LITTLEFS_FOR_IDF_3_2
#include <time.h>
#endif

#define FORMAT_LITTLEFS_IF_FAILED true

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
Adafruit_BME280 bme;

#include "utils.h"

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Variables to save values from HTML form
String ssid;
String pass;
String delay_time;
String station_name;
String urls;

float temp;
float pres;
float humi;

// File paths to save input values permanently
const char *ssidPath = "/ssid.txt";
const char *passPath = "/pass.txt";
const char *delay_timePath = "/delay_time.txt";
const char *station_namePath = "/station_name.txt";
const char *urlsPath = "/urls.txt";

// Timer variables
unsigned long previousMillis = 0;
const long interval = 10000; // interval to wait for Wi-Fi connection (milliseconds)

String ap_name = "rpc_meteo_" + String(ESP.getEfuseMac(), HEX);

#include <esp_task_wdt.h>
#define WDT_TIMEOUT 120

#define LED_BUILDTIN 2

// Initialize Filesystem
void initLittleFS()
{
  if (!LITTLEFS.begin(FORMAT_LITTLEFS_IF_FAILED))
  {
    Serial.println("LITTLEFS Mount Failed");
    return;
  }
  Serial.println("LittleFS mounted successfully");
}

void load_info() {
  ssid = readFile(LITTLEFS, ssidPath);
  pass = readFile(LITTLEFS, passPath);
  station_name = readFile(LITTLEFS, station_namePath);
  delay_time = readFile(LITTLEFS, delay_timePath);
  urls = readFile(LITTLEFS, urlsPath);

  if (delay_time.toInt() == 0) delay_time = "30";
  if (station_name == "") station_name = WiFi.macAddress();

  Serial.println("SSID: " + ssid);
  Serial.println("Password: " + pass);
  Serial.println("Station Name: " + station_name);
  Serial.println("Delay time: " + delay_time);
  Serial.println("URLs: " + urls);
}

// Initialize WiFi
bool initWiFi()
{
  if (ssid == "")
  {
    Serial.println("Undefined SSID or IP address.");
    return false;
  }

  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.println("Connecting to WiFi...");

  unsigned long currentMillis = millis();
  previousMillis = currentMillis;

  while (WiFi.status() != WL_CONNECTED)
  {
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval)
    {
      Serial.println("Failed to connect.");
      return false;
    }
  }

  Serial.println(WiFi.localIP());

  if(!MDNS.begin(ap_name)) {
    Serial.println("Error starting mDNS");
  }
  else {
    Serial.println("Successfully started mDNS: " + ap_name + ".local");
  }

  return true;
}

String processor(const String &var)
{
  if (var == "STATION_NAME") {
    return station_name;
  }
  if (var == "DELAY_TIME") {
    return delay_time;
  }
  if (var == "URLS") {
    return urls;
  }
  if (var == "PRES") {
    return String(pres);
  }
  if (var == "TEMP") {
    return String(temp);
  }
  if (var == "HUMI") {
    return String(humi);
  }
  return String();
}

void callback_configure_wifi(AsyncWebServerRequest *request) {
  int params = request->params();
  for(int i=0;i<params;i++){
    AsyncWebParameter* p = request->getParam(i);
    if(p->isPost()){
      // HTTP POST ssid value
      if (p->name() == "ssid") {
        ssid = p->value().c_str();
        Serial.print("SSID set to: ");
        Serial.println(ssid);
        // Write file to save value
        writeFile(LITTLEFS, ssidPath, ssid.c_str());
      }
      // HTTP POST pass value
      if (p->name() == "pass") {
        pass = p->value().c_str();
        Serial.print("Password set to: ");
        Serial.println(pass);
        // Write file to save value
        writeFile(LITTLEFS, passPath, pass.c_str());
      }
    }
  }
  request->send(200, "text/plain", "Done. ESP will restart");
  delay(3000);
  ESP.restart();
}

void callback_configure_station(AsyncWebServerRequest *request) {
  int params = request->params();
  for(int i=0;i<params;i++){
    AsyncWebParameter* p = request->getParam(i);
    if(p->isPost()){
      if (p->name() == "station_name") {
        station_name = p->value().c_str();
        Serial.print("Station Name set to: ");
        Serial.println(station_name);
        // Write file to save value
        writeFile(LITTLEFS, station_namePath, station_name.c_str());
      }
      if (p->name() == "delay_time") {
        delay_time = p->value().c_str();
        Serial.print("Station Name set to: ");
        Serial.println(delay_time);
        // Write file to save value
        writeFile(LITTLEFS, delay_timePath, delay_time.c_str());
      }
      if (p->name() == "urls") {
        urls = p->value().c_str();
        Serial.print("Station Name set to: ");
        Serial.println(urls);
        // Write file to save value
        writeFile(LITTLEFS, urlsPath, urls.c_str());
      }
      
    }
  }
  load_info();
  request->send(LITTLEFS, "/config.html", "text/html", false, processor);
}

void callback_delete_wifi_info(AsyncWebServerRequest *request) {
  deleteFile(LITTLEFS, ssidPath);
  deleteFile(LITTLEFS, passPath);
  request->send(200, "text/plain", "Done. ESP will restart");
  delay(3000);
  ESP.restart();
}

void set_wifi_configuration() {
  // Connect to Wi-Fi network with SSID and password
    Serial.println("Setting AP (Access Point)");
    // NULL sets an open Access Point

    WiFi.softAP(ap_name.c_str(), NULL);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);

    if(!MDNS.begin(ap_name)) {
      Serial.println("Error starting mDNS");
    }
    // Web Server Root URL
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LITTLEFS, "/wifimanager.html", "text/html"); });

    server.serveStatic("/", LITTLEFS, "/");

    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request)
              {
      callback_configure_wifi(request);
              });
    server.begin();
}

void setup()
{
  // Serial port for debugging purposes
  Serial.begin(115200);

  initLittleFS();
  
  // Load values saved in SPIFFS
  load_info();

  pinMode(LED_BUILDTIN, OUTPUT);

  bool status = bme.begin(0x76);
  if (status)
  {
    Serial.println("BME280 sensor found!");
  }
  else
  {
    Serial.println("Could not find a valid BME280 sensor, check wiring! Resetting in 5 s...");
    unsigned long time_now = millis();
    while (millis() - time_now < 5000)
    {
    }
    ESP.restart();
  }

  temp = bme.readTemperature();
  pres = bme.readPressure() / 100.0F;
  humi = bme.readHumidity();

  if (initWiFi())
  {
    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LITTLEFS, "/config.html", "text/html", false, processor); });
    server.serveStatic("/", LITTLEFS, "/");
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request)
              {
                callback_configure_station(request);
              });
    server.begin();
    server.on("/delete", HTTP_POST, [](AsyncWebServerRequest *request)
              {
                callback_delete_wifi_info(request);
              });
    server.begin();
  }
  else
  {
    set_wifi_configuration();
  }
  Serial.println("Setup finished!");
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
}

void send_data()
{
  // send the data to registered urls

  esp_task_wdt_reset();

  // save the sensor values.
  temp = bme.readTemperature();
  pres = bme.readPressure() / 100.0F;
  humi = bme.readHumidity();

  if (temp < -100.0)
  {
    Serial.println("Problem in the sensor, check connections. Resetting in 5 s...");
    unsigned long time_now = millis();
    while (millis() - time_now < 5000)
    {
    }
    ESP.restart();
  }

  // format json string
  String json = "{\"name\": \"" + station_name + "\", \"temp\": " + (String)temp + ", \"pres\": " + (String)pres + ", \"humi\": " + (String)humi + "}";

  Serial.println(json);

  digitalWrite(LED_BUILDTIN, HIGH);

  // send data via HTTP POST loop over the urls and send the data.
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    //bool finished = false;
    int i = 0;

    while (true) {
      String url = getValue(urls, '\n', i);
      if (url == "") {
        break;
      }
      Serial.print("Sending data to: ");
      Serial.println(url);
      int http_response;
      http.begin(url.c_str());
      http_response = http.POST(json.c_str());
      http.end();
      Serial.print("HTTP Response code: ");
      Serial.println(http_response);

      i+=1;
    }

  }
  else
  {
    // connect if not connected
    initWiFi();
  }
  digitalWrite(LED_BUILDTIN, LOW);
}

void loop()
{
  if ((millis() - previousMillis) > delay_time.toInt()*1000)
  {
    previousMillis = millis();
    send_data();
  }
}