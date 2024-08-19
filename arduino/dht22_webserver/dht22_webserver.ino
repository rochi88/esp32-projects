#include <WiFi.h>
#include <WebServer.h>
#include "DHT.h"

#define DHTPIN 10     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22   // DHT 22 (AM2302), AM2321

/* Put your SSID & Password */
const char* ssid = "Your WiFi Name";       // Enter your WiFi SSID
const char* password = " Your WiFi Password";     // Enter your WiFi password

WebServer server(80); // Create a web server object that listens for HTTP requests on port 80

DHT dht(DHTPIN, DHTTYPE);

float Temperature;
float Humidity;

#define LED_PIN 8

void setup() {
  Serial.begin(115200);
  Serial.println(F("DHTxx test with web server!"));
  dht.begin();
  pinMode(LED_PIN, OUTPUT);

  Serial.println("Connecting to wifi");
  WiFi.begin(ssid, password); // Connect to Wi-Fi network

  // Wait until the device is connected to Wi-Fi
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP()); // Print the IP address

  server.on("/", handle_OnConnect); // Define handling function for root URL
  server.onNotFound(handle_NotFound); // Define handling function for 404 (Not Found)

  server.begin(); // Start the HTTP server
  Serial.println("HTTP server started");
  
}

void loop() {
  server.handleClient(); // Handle client requests
}

// Handle connection to the root URL
void handle_OnConnect() {
  Temperature = dht.readTemperature();  // Get temperature value
  Humidity = dht.readHumidity();        // Get humidity value

  // Check if any reading failed and send "N/A" if so
  if (isnan(Temperature) || isnan(Humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    server.send(200, "text/html", SendHTML("N/A", "N/A"));
  } else {
    Serial.print("Temperature: ");
    Serial.println(Temperature);
    Serial.print("Humidity: ");
    Serial.println(Humidity);
    server.send(200, "text/html", SendHTML(String(Temperature), String(Humidity))); // Send the HTML page with the values
  }
}

// Handle not found URL
void handle_NotFound() {
  server.send(404, "text/plain", "Not found");
}

// Generate the HTML content to display
String SendHTML(String Temperaturestat, String Humiditystat) {
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