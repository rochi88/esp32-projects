#define ESP32_RTOS  

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#include "Adafruit_Sensor.h"
#include "Adafruit_AM2320.h"

#include "credentials.h"

#define LED_PIN 8
// Data wire is plugged into port 4 on the ESP32
#define ONE_WIRE_BUS 10

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

Adafruit_AM2320 am2320 = Adafruit_AM2320();

// Create AsyncWebServer object on port 80
WebServer server(80);

StaticJsonDocument<250> jsonDocument;
char buffer[250];

float temperature;
float humidity;
float dtemperatureC;
 
void setup_routing() {     
  server.on("/", handle_OnConnect);   // Define handling function for root URL
  server.onNotFound(handle_NotFound); // Define handling function for 404 (Not Found)
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
     digitalWrite(LED_PIN, HIGH); // Turn the LED LED white
     temperature = am2320.readTemperature();
     humidity = am2320.readHumidity();
     Serial.println("Read sensor data");
     sensors.requestTemperatures(); // Send the command to get temperatures
     dtemperatureC = sensors.getTempCByIndex(0);
     digitalWrite(LED_PIN, LOW); // Turn the LED LED off
 
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

void handle_OnConnect()
{
  // Check if any reading failed and send "N/A" if so
  if (isnan(temperature) || isnan(humidity))
  {
    Serial.println("Failed to read from AM2320 sensor!");
    server.send(200, "text/html", SendHTML("N/A", "N/A", "N/A"));
  }
  else
  {
    Serial.print("Temperature: ");
    Serial.println(temperature);
    Serial.print("Humidity: ");
    Serial.println(humidity);
    server.send(200, "text/html", SendHTML( String(dtemperatureC), String(temperature), String(humidity))); // Send the HTML page with the values
  }
}

// Handle not found URL
void handle_NotFound()
{
  server.send(404, "text/plain", "Not found");
}

// Generate the HTML content to display
String SendHTML(String dTemperaturestat, String Temperaturestat, String Humiditystat)
{
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<title>ESP32 Temperature & Humidity</title>\n";
  ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr += "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;}\n";
  ptr += "p {font-size: 24px;color: #444444;margin-bottom: 10px;}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<div id=\"webpage\">\n";
  ptr += "<h1>ESP32 Temperature & Humidity</h1>\n";

  ptr += "<p>Dallas Temperature: ";
  ptr += dTemperaturestat;
  ptr += " &deg;C</p>";
  ptr += "<p>Temperature: ";
  ptr += Temperaturestat;
  ptr += " &deg;C</p>";
  ptr += "<p>Humidity: ";
  ptr += Humiditystat;
  ptr += "%</p>";

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

    Serial.println("Initializing Adafruit AM2320");
    am2320.begin();

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
