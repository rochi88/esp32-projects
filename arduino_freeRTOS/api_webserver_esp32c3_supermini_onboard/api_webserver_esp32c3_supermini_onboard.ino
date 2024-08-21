#define ESP32_RTOS  

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#include "credentials.h"

#define LED_PIN 8
// Data wire is plugged into port 4 on the ESP32
#define ONE_WIRE_BUS 10

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// Create AsyncWebServer object on port 80
WebServer server(80);

StaticJsonDocument<250> jsonDocument;
char buffer[250];

float dtemperatureC;
float onBoardTemperatureC;
 
void setup_routing() {     
  server.on("/", handle_OnConnect);   // Define handling function for root URL
  server.onNotFound(handle_NotFound); // Define handling function for 404 (Not Found)    
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
     sensors.requestTemperatures(); // Send the command to get temperatures
     dtemperatureC = sensors.getTempCByIndex(0);
     onBoardTemperatureC = temperatureRead();    
 
     vTaskDelay(10000 / portTICK_PERIOD_MS);
   }
}

void TaskWriteToSerial(void * parameter) {  
   for (;;) {
     if(onBoardTemperatureC > 45.0){
        digitalWrite(LED_PIN, LOW);
        Serial.println(onBoardTemperatureC);
     } else {
        digitalWrite(LED_PIN, HIGH);
     }
 
     vTaskDelay(1000 / portTICK_PERIOD_MS);     
   }
}
 
void getData() {
  Serial.println("Get Sensor Data");
  jsonDocument.clear();
  add_json_object("temperature", onBoardTemperatureC, "°C");
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

void handle_OnConnect()
{
  // Check if any reading failed and send "N/A" if so
  if (isnan(onBoardTemperatureC) || isnan(dtemperatureC))
  {
    Serial.println("Failed to read from sensor!");
    server.send(200, "text/html", SendHTML("N/A", "N/A"));
  }
  else
  {
    Serial.print("Temperature: ");
    Serial.println(dtemperatureC);
    Serial.print("On-Board Temperature: ");
    Serial.println(onBoardTemperatureC);
    server.send(200, "text/html", SendHTML( String(dtemperatureC), String(onBoardTemperatureC))); // Send the HTML page with the values    
  }
}

// Handle not found URL
void handle_NotFound()
{
  server.send(404, "text/plain", "Not found");
}

// Generate the HTML content to display
String SendHTML(String dTemperaturestat, String Temperaturestat)
{
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<title>ESP32 Temperature</title>\n";
  ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr += "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;}\n";
  ptr += "p {font-size: 24px;color: #444444;margin-bottom: 10px;}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<div id=\"webpage\">\n";
  ptr += "<h1>ESP32 Temperature</h1>\n";

  ptr += "<p>Dallas Temperature: ";
  ptr += dTemperaturestat;
  ptr += " &deg;C</p>";
  ptr += "<p>On-Board Temperature: ";
  ptr += Temperaturestat;
  ptr += " &deg;C</p>";

  ptr += "</div>\n";
  ptr += "</body>\n";
  ptr += "</html>\n";
  return ptr;
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

  xTaskCreate(
    TaskWriteToSerial, 
    "Task Write To Serial",  // A name just for humans
    2048,  // The stack size can be checked by calling `uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);`
    NULL,  // No parameter is used
    2,  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    NULL  // Task handle is not used here
  );
}


void setup() {
  // put your setup code here, to run once:
    Serial.begin(115200);
    while (!Serial) {
      delay(10); // hang out until serial port opens
    }

    pinMode(LED_PIN, OUTPUT); // Initialize the LED_BUILTIN pin as an output

    Serial.println("Initializing Dallas Temperature IC");
    sensors.begin();

    WiFi.begin(mySSID, myPASSWORD);
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
#ifdef defined(ESP32_RTOS) && defined(ESP32)
#else
  server.handleClient(); // Handle client requests
#endif
}
