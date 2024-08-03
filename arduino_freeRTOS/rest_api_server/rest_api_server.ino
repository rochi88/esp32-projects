#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"

#include "Adafruit_Sensor.h"
#include "Adafruit_AM2320.h"

#include "secrets.h"  // add WLAN Credentials in here.

Adafruit_AM2320 am2320 = Adafruit_AM2320();

WebServer server(80);

StaticJsonDocument<250> jsonDocument;
char buffer[250];

float temperature;
float humidity;
 
void setup_routing() {     
  server.on("/temperature", getTemperature);        
  server.on("/humidity", getHumidity);     
  server.on("/data", getData);     
  server.on("/led", HTTP_POST, handlePost);    
          
  server.begin();    
}
 
void create_json(char *tag, float value, char *unit) {  
  jsonDocument.clear();  
  jsonDocument["type"] = tag;
  jsonDocument["value"] = value;
  jsonDocument["unit"] = unit;
  serializeJson(jsonDocument, buffer);
}
 
void add_json_object(char *tag, float value, char *unit) {
  JsonObject obj = jsonDocument.createNestedObject();
  obj["type"] = tag;
  obj["value"] = value;
  obj["unit"] = unit; 
}

void read_sensor_data(void * parameter) {
   for (;;) {
     temperature = am2320.readTemperature();
     humidity = am2320.readHumidity();
     Serial.println("Read sensor data");
 
     vTaskDelay(60000 / portTICK_PERIOD_MS);
   }
}
 
void getTemperature() {
  Serial.println("Get temperature");
  create_json("temperature", temperature, "°C");
  server.send(200, "application/json", buffer);
}
 
void getHumidity() {
  Serial.println("Get humidity");
  create_json("humidity", humidity, "%");
  server.send(200, "application/json", buffer);
}
 
void getData() {
  Serial.println("Get AM2320 Sensor Data");
  jsonDocument.clear();
  add_json_object("temperature", temperature, "°C");
  add_json_object("humidity", humidity, "%");
  serializeJson(jsonDocument, buffer);
  server.send(200, "application/json", buffer);
}

void handlePost() {
  if (server.hasArg("plain") == false) {
  }
  String body = server.arg("plain");
  deserializeJson(jsonDocument, body);

  Serial.println("Received POST request");

  const char* data = jsonDocument["sensor"];
  Serial.println(data);

  server.send(200, "application/json", "{}");
}

void setup_task() {    
  xTaskCreate(     
  read_sensor_data,      
  "Read sensor data",      
  1000,      
  NULL,      
  1,     
  NULL     
  );     
}


void setup() {
  // put your setup code here, to run once:

    Serial.begin(115200);
    while (!Serial) {
      delay(10); // hang out until serial port opens
    }

    Serial.println("Adafruit AM2320 Basic Test");
    am2320.begin();
//    if (!am2320.begin(0x5c)) {    
//      Serial.println("AM2320 not found! Check Circuit");    
//    }   

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

void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient(); 
}