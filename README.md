# Simple Meteo Station using esp32 + BME280

Simple meteo station to measure enviromental parameters (pressure, temperature and relative humidity).

The microcontroller is connected to the sensor BME280 using I2C and the data is collected frequently in a configurable time.

The env. parameters are sent via HTTP POST request to a server which will archive the data.

## Connections

| BME280      | ESP32       |
| ----------- | ----------- |
| VCC         | 3.3 V       |
| GND         | GND         |
| SDA         | GPIO21      |
| SCL         | GPIO22      |

## Flashing the firmware

You should use the [PlatformIO](https://platformio.org). 

## Connection via serial

Use the tool `serial_monitor.py` to monitor the serial connection and to send commands to the controller. pyserial must be installed. Run it using 

```
python serial_monitor.py -p "port"
````

you should change the "port" to the one on your PC. To send commands via serial, simply type the command and hit enter to send it.

| Command          | Description                                       |
| ---------------- | ------------------------------------------------- |
| config_network   | Redefine the network credentials (only if not connecting to eduroam)     |
| config_url       | Save the urls for sending the data                |
| config_boardname | Configure the board name                          |
| config_delay     | Configure the delay time between data sending     |
| erase_flash      | Erase all saved configurations                    |

## First power up

The first time the board is powered, it will ask the network credentials to connect to WIFI. Use the `serial_monitor.py` tool to check the messages from serial. When asked, tap the ssid and the password.

After that, it will try to connect to the network. If you have any problems check the network credentials.

## HTTP POST

The POST body is a JSON with the following body:

```
{"name": str("board_name"), "temp": float(T in °C), "pres": float(P in mbar), "humi": float(RH in %)}
```

it will be sent regularly within a delay time (default=30s), to the registered URLs (up to 5), if no URL is registered, it will only fetch the sensor data and print to serial, but no POST request will be sent. Register the URLs using the command `config_url`

## Problems

Try to erase the esp32 flash and reflashing the firmware. Do all the configuration steps and try again.