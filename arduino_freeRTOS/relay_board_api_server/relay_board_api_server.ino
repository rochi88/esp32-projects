#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"

#include "secrets.h" // add WLAN Credentials in here.

// Pin assignment (see README.md)
#define PIN_RELAY1 32
#define PIN_RELAY2 33
#define PIN_RELAY3 25
#define PIN_RELAY4 26
#define PIN_LED    23

// Most ESP32 4-relay boards drive the relay through a transistor (active HIGH).
// Set to LOW if your relays click on when the pin is pulled low.
#define RELAY_ACTIVE_LEVEL HIGH

#define RELAY_COUNT 4
#define PULSE_MAX_MS 600000

const uint8_t relayPins[RELAY_COUNT] = {PIN_RELAY1, PIN_RELAY2, PIN_RELAY3, PIN_RELAY4};
bool relayState[RELAY_COUNT] = {false, false, false, false};
TimerHandle_t pulseTimers[RELAY_COUNT];
SemaphoreHandle_t relayMutex;

WebServer server(80);

StaticJsonDocument<512> jsonDocument;
char buffer[512];

// ---------------------------------------------------------------------------
// Relay helpers
// ---------------------------------------------------------------------------

void set_relay(uint8_t index, bool on)
{
    xSemaphoreTake(relayMutex, portMAX_DELAY);
    relayState[index] = on;
    digitalWrite(relayPins[index], on ? RELAY_ACTIVE_LEVEL : !RELAY_ACTIVE_LEVEL);
    xSemaphoreGive(relayMutex);

    Serial.printf("Relay %d -> %s\n", index + 1, on ? "ON" : "OFF");
}

bool get_relay(uint8_t index)
{
    xSemaphoreTake(relayMutex, portMAX_DELAY);
    bool on = relayState[index];
    xSemaphoreGive(relayMutex);
    return on;
}

// One-shot timer callback: turns the relay back off after a pulse
void pulse_timer_callback(TimerHandle_t timer)
{
    uint8_t index = (uint32_t)pvTimerGetTimerID(timer);
    set_relay(index, false);
}

void start_pulse(uint8_t index, uint32_t ms)
{
    set_relay(index, true);
    xTimerChangePeriod(pulseTimers[index], pdMS_TO_TICKS(ms), portMAX_DELAY);
    xTimerStart(pulseTimers[index], portMAX_DELAY);
}

void cancel_pulse(uint8_t index)
{
    xTimerStop(pulseTimers[index], portMAX_DELAY);
}

void setup_relays()
{
    relayMutex = xSemaphoreCreateMutex();

    for (uint8_t i = 0; i < RELAY_COUNT; i++)
    {
        pinMode(relayPins[i], OUTPUT);
        digitalWrite(relayPins[i], !RELAY_ACTIVE_LEVEL);
        pulseTimers[i] = xTimerCreate("pulse", pdMS_TO_TICKS(1000), pdFALSE, (void *)(uint32_t)i, pulse_timer_callback);
    }
}

// ---------------------------------------------------------------------------
// JSON helpers
// ---------------------------------------------------------------------------

void add_relay_object(JsonArray arr, uint8_t index)
{
    JsonObject obj = arr.createNestedObject();
    obj["id"] = index + 1;
    obj["pin"] = relayPins[index];
    obj["state"] = get_relay(index) ? "on" : "off";
}

void send_error(int code, const char *message)
{
    jsonDocument.clear();
    jsonDocument["error"] = message;
    serializeJson(jsonDocument, buffer);
    server.send(code, "application/json", buffer);
}

void send_relay(uint8_t index)
{
    jsonDocument.clear();
    jsonDocument["id"] = index + 1;
    jsonDocument["pin"] = relayPins[index];
    jsonDocument["state"] = get_relay(index) ? "on" : "off";
    serializeJson(jsonDocument, buffer);
    server.send(200, "application/json", buffer);
}

void send_all_relays()
{
    jsonDocument.clear();
    JsonArray arr = jsonDocument.createNestedArray("relays");
    for (uint8_t i = 0; i < RELAY_COUNT; i++)
    {
        add_relay_object(arr, i);
    }
    serializeJson(jsonDocument, buffer);
    server.send(200, "application/json", buffer);
}

// Returns 0..RELAY_COUNT-1, or -1 if the 1-based id is invalid
int parse_relay_id(int id)
{
    if (id < 1 || id > RELAY_COUNT)
    {
        return -1;
    }
    return id - 1;
}

// Applies "on" / "off" / "toggle" (or true / false) to a relay.
// Returns false if the state value is not recognised.
bool apply_state(uint8_t index, JsonVariant state)
{
    bool on;
    if (state.is<bool>())
    {
        on = state.as<bool>();
    }
    else if (state.is<const char *>())
    {
        String s = state.as<const char *>();
        s.toLowerCase();
        if (s == "on")
            on = true;
        else if (s == "off")
            on = false;
        else if (s == "toggle")
            on = !get_relay(index);
        else
            return false;
    }
    else
    {
        return false;
    }

    cancel_pulse(index);
    set_relay(index, on);
    return true;
}

// ---------------------------------------------------------------------------
// HTTP handlers
// ---------------------------------------------------------------------------

// GET /relays
void getRelays()
{
    send_all_relays();
}

// GET /relay?id=1
void getRelay()
{
    int index = parse_relay_id(server.arg("id").toInt());
    if (index < 0)
    {
        send_error(400, "id must be 1-4");
        return;
    }
    send_relay(index);
}

// Loads the request into jsonDocument from either a JSON body
// (Content-Type: application/json) or form/query fields id, state, pulse_ms.
// WebServer only keeps the raw body as "plain" for non-form content types;
// curl -d without -H sends form-urlencoded, so the fields fallback covers it.
bool read_request()
{
    if (server.hasArg("plain"))
    {
        if (deserializeJson(jsonDocument, server.arg("plain")))
        {
            send_error(400, "invalid JSON");
            return false;
        }
        return true;
    }

    jsonDocument.clear();
    if (server.hasArg("id"))
        jsonDocument["id"] = server.arg("id").toInt();
    if (server.hasArg("state"))
        jsonDocument["state"] = server.arg("state");
    if (server.hasArg("pulse_ms"))
        jsonDocument["pulse_ms"] = server.arg("pulse_ms").toInt();

    if (jsonDocument.size() == 0)
    {
        send_error(400, "send JSON with Content-Type: application/json, or fields id/state/pulse_ms");
        return false;
    }
    return true;
}

// POST /relay  {"id": 1, "state": "on" | "off" | "toggle"}
//              {"id": 1, "pulse_ms": 500}
void postRelay()
{
    if (!read_request())
        return;

    int index = parse_relay_id(jsonDocument["id"] | 0);
    if (index < 0)
    {
        send_error(400, "id must be 1-4");
        return;
    }

    if (jsonDocument.containsKey("pulse_ms"))
    {
        uint32_t ms = jsonDocument["pulse_ms"] | 0;
        if (ms == 0 || ms > PULSE_MAX_MS)
        {
            send_error(400, "pulse_ms must be 1-600000");
            return;
        }
        start_pulse(index, ms);
    }
    else if (!apply_state(index, jsonDocument["state"]))
    {
        send_error(400, "state must be on, off, toggle or a boolean");
        return;
    }

    send_relay(index);
}

// POST /relays  {"state": "on" | "off" | "toggle"}
void postRelays()
{
    if (!read_request())
        return;

    JsonVariant state = jsonDocument["state"];
    for (uint8_t i = 0; i < RELAY_COUNT; i++)
    {
        if (!apply_state(i, state))
        {
            send_error(400, "state must be on, off, toggle or a boolean");
            return;
        }
    }

    send_all_relays();
}

void notFound()
{
    send_error(404, "not found");
}

void setup_routing()
{
    server.on("/relays", HTTP_GET, getRelays);
    server.on("/relays", HTTP_POST, postRelays);
    server.on("/relay", HTTP_GET, getRelay);
    server.on("/relay", HTTP_POST, postRelay);
    server.onNotFound(notFound);

    server.begin();
}

// ---------------------------------------------------------------------------
// Tasks
// ---------------------------------------------------------------------------

// Status LED: fast blink while WiFi is down (and reconnects), solid when connected
void wifi_status_task(void *parameter)
{
    uint32_t lastReconnect = 0;
    for (;;)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            digitalWrite(PIN_LED, HIGH);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        else
        {
            digitalWrite(PIN_LED, !digitalRead(PIN_LED));
            if (millis() - lastReconnect > 10000)
            {
                Serial.println("WiFi lost, reconnecting");
                WiFi.reconnect();
                lastReconnect = millis();
            }
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
}

void setup_task()
{
    xTaskCreate(
        wifi_status_task,
        "WiFi status",
        2048,
        NULL,
        1,
        NULL);
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        delay(10); // hang out until serial port opens
    }

    // Drive relays to a known OFF state before anything else
    setup_relays();
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LOW);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.println("Connecting");
    while (WiFi.status() != WL_CONNECTED)
    {
        digitalWrite(PIN_LED, !digitalRead(PIN_LED));
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());

    setup_task();
    setup_routing();
}

void loop()
{
    server.handleClient();
}
