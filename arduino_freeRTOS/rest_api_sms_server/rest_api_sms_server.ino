#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"

#include "secrets.h"  // add WLAN Credentials in here.

#include "modem.h"

WebServer server(80);

StaticJsonDocument<250> jsonDocument;
char buffer[250];

void setup_routing() {       
  server.on("/data", HTTP_POST, handlePost);    
          
  server.begin();    
}

void read_data(void * parameter) {
   for (;;) {
     Serial.println("Read data");
     modem.maintain();
 
     vTaskDelay(60000 / portTICK_PERIOD_MS);
   }
}

void handlePost() {
  if (server.hasArg("plain") == false) {
  }
  String body = server.arg("plain");
  deserializeJson(jsonDocument, body);

  Serial.println("Received POST request");

  const char* data = jsonDocument["sensor"];
  Serial.println(data);

    SerialMon.println("Initializing modem...");
    modem.init();

   String imei = modem.getIMEI();
   DBG("IMEI:", imei);

   bool  res = modem.sendSMS(data, String("Hello from ") + imei);
   DBG("SMS:", res ? "OK" : "fail");

   server.send(200, "application/json", "{}");
}

void setup_task() {    
  xTaskCreate(     
  read_data,      
  "Read data",      
  1000,      
  NULL,      
  1,     
  NULL     
  );     
}

void setup()
{
    // Set console baud rate
    SerialMon.begin(115200);

    delay(10);

    WiFi.begin(ssid, password);
    Serial.println("Connecting");
    while(WiFi.status() != WL_CONNECTED) {
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
