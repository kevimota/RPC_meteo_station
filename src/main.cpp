#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

#include <nvs_flash.h>
#include <Preferences.h>
Preferences preferences;

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
Adafruit_BME280 bme;

#include <esp_task_wdt.h>
#define WDT_TIMEOUT 120

#define LED_BUILDTIN 2

// Configuration values of the board
String ssid = "";
String password = "";
unsigned int delay_time;
String board_name = "";
String url[10] = {"", "", "", "", "", "", "", "", "", ""};

// for checking the time
unsigned long last_time;

String wait_serial()
{
  while (Serial.available() < 1)
  {
    delay(500);
  }
  return Serial.readStringUntil('\n');
}

void def_credentials()
{
  // save the credentials to flash memory
  Serial.println("Setup for the credentials.");
  Serial.println("Enter the ssid: ");
  ssid = wait_serial();

  Serial.println("Enter the password: ");
  password = wait_serial();

  preferences.begin("config", false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);

  Serial.println("Network credentials saved!");

  preferences.end();

  ESP.restart();
}

void network_connect()
{
  // connect to the registered network
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid.c_str(), password.c_str());
  unsigned long last_time = millis();

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    if ((millis() - last_time) > 50000)
    {
      // try to connect 50 s timeout
      Serial.println("");
      Serial.println("Timeout, configure the password again.");
      def_credentials();
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup()
{
  Serial.begin(115200);
  while (!Serial)
  {
    delay(10);
  }
  Serial.println("Starting the setup:");

  pinMode(LED_BUILDTIN, OUTPUT);

  // get configuration data from flash memory
  preferences.begin("config", false);
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");
  delay_time = preferences.getUInt("delay_time", 0);
  board_name = preferences.getString("board_name", "meteo_station");
  url[0] = preferences.getString("url0", "");
  url[1] = preferences.getString("url1", "");
  url[2] = preferences.getString("url2", "");
  url[3] = preferences.getString("url3", "");
  url[4] = preferences.getString("url4", "");
  url[5] = preferences.getString("url5", "");
  url[6] = preferences.getString("url6", "");
  url[7] = preferences.getString("url7", "");
  url[8] = preferences.getString("url8", "");
  url[9] = preferences.getString("url9", "");
  preferences.end();

  if (delay_time < 1000)
  {
    Serial.println("Delay time less than 1s, setting the default: 30 s");
    delay_time = 30000;
  }

  if (ssid == "")
  {
    Serial.println("No credentials saved!");
    def_credentials();
  }

  network_connect(); // connect to network

  // configure and check sensor
  bool status;
  status = bme.begin(0x76);
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

  Serial.println("Setup finished!");
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
}

void send_data()
{
  // send the data to registered urls

  esp_task_wdt_reset();

  // save the sensor values.
  float temp = bme.readTemperature();
  float pres = bme.readPressure() / 100.0F;
  float humi = bme.readHumidity();

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
  String json = "{\"name\": \"" + board_name + "\", \"temp\": " + (String)temp + ", \"pres\": " + (String)pres + ", \"humi\": " + (String)humi + "}";

  Serial.println(json);

  digitalWrite(LED_BUILDTIN, HIGH);

  // send data via HTTP POST loop over the urls and send the data.
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    for (int i = 0; i < 5; ++i)
    {
      if (url[i] == "")
      {
        continue;
      }
      Serial.print("Sending data to: ");
      Serial.println(url[i]);
      int http_response;
      http.begin(url[i].c_str());
      http_response = http.POST(json.c_str());
      http.end();
      Serial.print("HTTP Response code: ");
      Serial.println(http_response);
    }
  }
  else
  {
    // connect if not connected
    network_connect();
  }
  digitalWrite(LED_BUILDTIN, LOW);
}

void flash_url(String key, int number)
{
  // save urls in flash
  Serial.println("Enter the new url:");
  String new_url = wait_serial();
  preferences.putString(key.c_str(), new_url.c_str());
  url[number] = new_url;
  Serial.println("Saved!");
}

void save_urls()
{
  // check and save urls for http post
  Serial.println("Currently saved urls are: ");
  for (int i = 0; i < 5; ++i)
  {
    Serial.print(i + 1);
    Serial.print("- ");
    if (url[i] == "")
      Serial.println("Not set!");
    else
      Serial.println(url[i]);
  }
  bool looping = false;
  Serial.println("Do you want to change the URLs? [y/n]");
  if (wait_serial() == "y")
    looping = true;
  preferences.begin("config", false);
  while (looping)
  {
    Serial.println("Choose the URL to change: [0-9]");
    int number = wait_serial().toInt();
    switch (number)
    {
    case 0:
      flash_url("url0", 0);
      break;
    case 1:
      flash_url("url1", 1);
      break;
    case 2:
      flash_url("url2", 2);
      break;
    case 3:
      flash_url("url3", 3);
      break;
    case 4:
      flash_url("url4", 4);
      break;
    case 5:
      flash_url("url0", 5);
      break;
    case 6:
      flash_url("url1", 6);
      break;
    case 7:
      flash_url("url2", 7);
      break;
    case 8:
      flash_url("url3", 8);
      break;
    case 9:
      flash_url("url4", 9);
      break;

    default:
      Serial.print("URL ");
      Serial.print(number);
      Serial.println(" does not exist!");
    }
    Serial.println("Do you want to change the URLs? [y/n]");
    if (wait_serial() == "n")
      looping = false;
  }
  preferences.end();
}

void set_boardname()
{
  // Set the board name
  Serial.print("Current board name is: ");
  Serial.println(board_name);
  Serial.println("Do you want to change the board name [y/n]?");
  if (wait_serial() == "y")
  {
    Serial.println("Set new board name: ");
    board_name = wait_serial();
  }
  preferences.begin("config", false);
  preferences.putString("board_name", board_name);
  preferences.end();
  Serial.println("Saved!");
}

void set_delay()
{
  // Set delay time in seconds, it is converted to millisecond.
  Serial.print("Current delay time is: ");
  Serial.print(delay_time / 1000.);
  Serial.println(" s");
  Serial.println("Do you want to change the delay time [y/n]?");
  if (wait_serial() == "y")
  {
    Serial.println("Set new delay time: ");
    delay_time = wait_serial().toInt() * 1000;
  }

  if (delay_time < 1000)
  {
    Serial.println("Delay time less than 1s, setting the default: 30 s");
    delay_time = 30000;
  }
  preferences.begin("config", false);
  preferences.putUInt("delay_time", delay_time);
  preferences.end();
  Serial.println("Saved!");
}

void erase_flash()
{
  nvs_flash_erase();
  nvs_flash_init();
  ESP.restart();
}

void loop()
{
  // send data after delay_time passed
  if ((millis() - last_time) > delay_time)
  {
    last_time = millis();
    send_data();
  }

  // configuration options
  if (Serial.available() > 0)
  {
    String read_data = Serial.readStringUntil('\n');
    if (read_data == "config_network")
      def_credentials();
    else if (read_data == "config_url")
      save_urls();
    else if (read_data == "config_boardname")
      set_boardname();
    else if (read_data == "config_delay")
      set_delay();
    else if (read_data == "erase_flash")
      erase_flash();
  }
}