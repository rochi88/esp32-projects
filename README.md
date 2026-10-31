# esp32-projects

ESP32 lab exercises and small firmware projects: sensors, local web servers, JSON REST APIs, relays, OTA, SMS and Firebase logging. Sketches use the Arduino core (plain and FreeRTOS); there are also Zephyr RTOS projects.

These are standalone learning and prototyping projects, not part of an Oxidex product. The production edge firmware for BDShare lives in [esp32-bdshare](../esp32-bdshare/).

---

## Layout

```text
esp32-projects/
├── arduino/            Single-loop Arduino sketches (sensors, simple web servers)
├── arduino_freeRTOS/   Arduino + FreeRTOS sketches (REST APIs, relays, OTA, SMS, Firebase)
├── zephyr/             Zephyr RTOS boards and projects (west)
└── doc/                Pinouts and board notes (ESP32-C3 SuperMini, NodeMCU ESP8266, LilyGo T-Call SIM800, TTGO/Blynk)
```

---

## Projects

### `arduino/`

| Sketch | What it does | Hardware / libraries |
| --- | --- | --- |
| [BlinkLED](arduino/BlinkLED/) | Blink the on-board LED | any ESP32 |
| [BlinkLED_native_temperature](arduino/BlinkLED_native_temperature/) | Blink and print the chip's internal temperature sensor | ESP32 with internal temp sensor (e.g. C3) |
| [dht22](arduino/dht22/) | Read temperature/humidity to Serial | DHT22 on GPIO 10, LED on GPIO 8 · `DHT` |
| [dht22_webserver](arduino/dht22_webserver/) | DHT22 readings on an HTML page at `/` | DHT22 GPIO 10 · `WiFi`, `WebServer` |
| [ds18b20](arduino/ds18b20/) | Read one DS18B20 to Serial ([README](arduino/ds18b20/README.md)) | DS18B20 on GPIO 4 + 4.7 kΩ pull-up · `OneWire`, `DallasTemperature` |
| [ds18b20_multiple](arduino/ds18b20_multiple/) | Discover and read several DS18B20s on one bus | as above |
| [ds18b20_webserver](arduino/ds18b20_webserver/) | Async web page plus `/temperaturec` and `/temperaturef` ([README](arduino/ds18b20_webserver/README.md)) | DS18B20 GPIO 4 · `ESPAsyncWebServer`, `AsyncTCP` |

### `arduino_freeRTOS/`

| Sketch | What it does | REST routes |
| --- | --- | --- |
| [rest_api_server](arduino_freeRTOS/rest_api_server/) | JSON API for sensor values and LED control ([README](arduino_freeRTOS/rest_api_server/README.md)) | `/temperature`, `/humidity`, `/data`, `/led` |
| [rest_api_server_and_firebase_logging](arduino_freeRTOS/rest_api_server_and_firebase_logging/) | Same API plus an HTTPS client (`WiFiClientSecure` + `HTTPClient`); currently calls a TLS test endpoint, and the Firebase upload is still to do ([README](arduino_freeRTOS/rest_api_server_and_firebase_logging/README.md)) | `/temperature`, `/humidity`, `/data`, `/led` |
| [rest_api_sms_server](arduino_freeRTOS/rest_api_sms_server/) | JSON API on a SIM800 board; sends SMS via TinyGSM ([README](arduino_freeRTOS/rest_api_sms_server/README.md)) | `/data` |
| [api_webserver_esp32c3_supermini](arduino_freeRTOS/api_webserver_esp32c3_supermini/) | DS18B20 JSON API for the ESP32-C3 SuperMini | `/`, `/temperature`, `/humidity`, `/data`, `/led` |
| [api_webserver_esp32c3_supermini_onboard](arduino_freeRTOS/api_webserver_esp32c3_supermini_onboard/) | No external sensor: reports the chip's internal temperature (`temperatureRead()`) | `/`, `/data`, `/led` |
| [dht22_webserver](arduino_freeRTOS/dht22_webserver/) | DHT22 web page as a FreeRTOS task | `/` |
| [BasicOTA](arduino_freeRTOS/BasicOTA/) | Template for over-the-air updates (`OTA.h`). Wi-Fi credentials come from `credentials.h` | n/a |
| [relay_board_api_server](arduino_freeRTOS/relay_board_api_server/) | 4-channel relay board over a JSON REST API on port 80, with a Makefile for compile/flash/monitor/backup ([README](arduino_freeRTOS/relay_board_api_server/README.md)) | `/relays`, `/relay` |

### `zephyr/`

| Project | What it does |
| --- | --- |
| [ESP32 Wroom Zephyr Project](zephyr/projects/ESP32%20Wroom%20Zephyr%20Project/) | Wi-Fi + HTTP server on `esp32_devkitc_wroom` ([README](zephyr/projects/ESP32%20Wroom%20Zephyr%20Project/README.md)) |
| [exercise](zephyr/projects/exercise/) | Minimal west workspace / app skeleton |
| [boards/espressif/esp32c3_supermini](zephyr/boards/espressif/) | Out-of-tree board definition for the ESP32-C3 SuperMini |

---

## Building

### Arduino sketches

1. Install the **esp32** board package (Espressif) in Arduino IDE, or use `arduino-cli`.
2. Install the libraries named in the table or in each sketch's README.
3. Set your Wi-Fi SSID and password in the sketch, or in `credentials.h` for BasicOTA. Keep real credentials out of git: add them locally and don't commit them.
4. Select the board (e.g. *ESP32 Dev Module*, *ESP32C3 Dev Module* for the SuperMini) and upload.

```bash
# arduino-cli example
arduino-cli compile --fqbn esp32:esp32:esp32 arduino_freeRTOS/rest_api_server
arduino-cli upload  --fqbn esp32:esp32:esp32 -p /dev/ttyUSB0 arduino_freeRTOS/rest_api_server
```

`relay_board_api_server` wraps this in a Makefile: `make help`, `make compile`, `make flash`, `make monitor`, `make backup`.

### Zephyr projects

```bash
cd "zephyr/projects/ESP32 Wroom Zephyr Project"
west build -b esp32_devkitc_wroom -- -DSSID="WIFI_SSID" -DPSK="WIFI_PASS"
west flash
```

See [zephyr/README.md](zephyr/README.md) for toolchain links.

---

## Links

- [Wokwi simulator](https://wokwi.com/): run most sketches without hardware
- [ESP32-C3 SuperMini resources](https://www.sudo.is/docs/esphome/boards/esp32c3supermini/)
- [Zephyr application development](https://docs.zephyrproject.org/latest/develop/application/index.html)

## License

See [LICENSE](LICENSE).
