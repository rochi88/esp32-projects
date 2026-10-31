# Relay board API server

ESP32 firmware (Arduino + FreeRTOS) that exposes a 4-channel relay board over a JSON REST API on port 80.

## Hardware

- Module: **ESP32-WROOM-32E** (ESP32-D0WD-V3, rev v3.1, 4MB flash)
- Arduino IDE board: **ESP32 Dev Module**, Flash Size **4MB (32Mb)**

## Pin assignment

| Function   | GPIO | Define       |
|------------|------|--------------|
| Relay 1    | 32   | `PIN_RELAY1` |
| Relay 2    | 33   | `PIN_RELAY2` |
| Relay 3    | 25   | `PIN_RELAY3` |
| Relay 4    | 26   | `PIN_RELAY4` |
| Status LED | 23   | `PIN_LED`    |

Relays are driven active HIGH by default. If your board switches on a LOW signal, change `RELAY_ACTIVE_LEVEL` in the sketch.

Status LED: blinks while WiFi is connecting or reconnecting, stays on once connected.

## Behaviour

- All relays are forced OFF at boot, before WiFi starts.
- A pulse (`pulse_ms`) uses a per-relay FreeRTOS one-shot timer; any later on/off/toggle on that relay cancels it.
- If WiFi drops, the board retries every 10 s. Relay states are kept.

## Partitions

`partitions.csv` in the sketch folder replaces the board's default table (Arduino-ESP32 core 2.x+ picks it up automatically). It targets the module's **4MB flash**:

| Name     | Type | SubType  | Offset   | Size   |
|----------|------|----------|----------|--------|
| nvs      | data | nvs      | 0x9000   | 20 KB  |
| factory  | app  | factory  | 0x10000  | 2 MB   |
| spiffs   | data | spiffs   | 0x210000 | 1.9 MB |
| coredump | data | coredump | 0x3F0000 | 64 KB  |

No OTA: a single factory app and no `otadata`, so the bootloader always boots `factory`. `spiffs` can hold either SPIFFS or LittleFS. Changing the table requires a full erase (`make deploy-clean`), which also wipes NVS.

## Setup

1. Copy `secrets.h.example` to `secrets.h` and fill in your WiFi credentials (`secrets.h` is gitignored).
2. Install the **ArduinoJson** library (v6 or v7).
3. Select **ESP32 Dev Module** with Flash Size **4MB**, then upload.
4. Read the IP address from the serial monitor (115200 baud).

### Flashing mode (IO0 jumper)

The board has no auto-reset circuit; boot mode is set by a jumper from GPIO0 to GND.

| Jumper IO0–GND | Boot message                         | Mode                      |
|----------------|--------------------------------------|---------------------------|
| On             | `boot:0x3 (DOWNLOAD_BOOT ...)`       | Waits for upload          |
| Off            | `boot:0x13 (SPI_FAST_FLASH_BOOT)`    | Runs the firmware         |

1. Fit the jumper, power-cycle, run `make deploy`.
2. Remove the jumper, power-cycle (or press EN/RST).

Power the board from its own supply input, not the USB-TTL adapter's 3.3V pin, otherwise the brownout detector resets it at boot (`E BOD: Brownout detector was triggered`). Connect only TX, RX and GND to the adapter.

### Command line (arduino-cli)

Same layout as `esp32-bdshare`: `scripts/setup_env.sh` checks and installs the toolchain, `scripts/deploy.sh` compiles and flashes, and the Makefile wraps both.

```sh
cp secrets.h.example secrets.h   # then fill in WiFi details
make setup-env                   # install arduino-cli (~/.local/bin), esp32 core, ArduinoJson
make check-env                   # same checks, install nothing
make compile                     # build into build/
make deploy                      # compile + upload (first /dev/ttyUSB*, or PORT=...)
make flash                       # deploy + serial monitor
make flash-id                    # chip type + flash size
make erase                       # wipe flash (after changing partitions.csv)

make status                      # GET /relays (IP defaults to 192.168.1.245)
make relay-on ID=2               # also relay-off, relay-toggle, relay ID=2
make relay-pulse ID=3 MS=500     # on, then off after 500 ms
make all-off                     # also all-on, all-toggle
make status IP=192.168.1.50      # other board address
```

## API

| Method | Endpoint      | Body / Query                                    | Description                                          |
|--------|---------------|-------------------------------------------------|------------------------------------------------------|
| GET    | `/relays`     | –                                               | State of all relays                                  |
| POST   | `/relays`     | `{"state": "on" \| "off" \| "toggle"}`          | Set all relays                                       |
| GET    | `/relay?id=N` | `id` = 1–4                                      | State of one relay                                   |
| POST   | `/relay`      | `{"id": N, "state": "on" \| "off" \| "toggle"}` | Set one relay (`state` may also be `true`/`false`)   |
| POST   | `/relay`      | `{"id": N, "pulse_ms": 500}`                    | Turn relay on, then off after `pulse_ms` (1–600000)  |

Errors return `400` (bad input) or `404` (unknown path) with a JSON `error` field.

### Examples

```sh
curl http://<ip>/relays
curl "http://<ip>/relay?id=2"
curl -X POST http://<ip>/relay  -H 'Content-Type: application/json' -d '{"id": 1, "state": "on"}'
curl -X POST http://<ip>/relay  -H 'Content-Type: application/json' -d '{"id": 3, "pulse_ms": 1000}'
curl -X POST http://<ip>/relays -H 'Content-Type: application/json' -d '{"state": "off"}'

# form / query fields work too (no JSON header needed)
curl -X POST http://<ip>/relay -d 'id=1&state=on'
curl -X POST "http://<ip>/relay?id=2&pulse_ms=500"
```

Without `-H 'Content-Type: application/json'`, `curl -d` sends the body as a form, and the ESP32 WebServer drops a JSON body sent that way.

### Responses

Single relay:

```json
{"id": 1, "pin": 32, "state": "on"}
```

All relays:

```json
{"relays": [
  {"id": 1, "pin": 32, "state": "on"},
  {"id": 2, "pin": 33, "state": "off"},
  {"id": 3, "pin": 25, "state": "off"},
  {"id": 4, "pin": 26, "state": "off"}
]}
```

Error:

```json
{"error": "id must be 1-4"}
```
