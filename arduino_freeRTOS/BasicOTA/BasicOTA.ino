#define ESP32_RTOS  // Uncomment this line if you want to use the code with freertos only on the ESP32
                    // Has to be done before including "OTA.h"

#include "OTA.h"
#include "credentials.h"

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");

  setupOTA("TemplateSketch", mySSID, myPASSWORD);

  // Your setup code
  pinMode(8, OUTPUT);
}

void loop() {
#ifdef defined(ESP32_RTOS) && defined(ESP32)
#else
  ArduinoOTA.handle();
#endif

  // Your code here
  digitalWrite(8, HIGH);
  delay(1000);
  
  digitalWrite(8, LOW);
  delay(1000);  
}