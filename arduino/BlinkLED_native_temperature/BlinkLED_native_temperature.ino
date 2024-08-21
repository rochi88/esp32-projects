/*
  BlinkLED

  Demonstrates usage of onboard LED on some ESP dev boards.
  Turns an LED on for two seconds, then off for one second, repeatedly.
  This is coded by Sidharth Mohan : https://github.com/sidharthmohannair/
*/
/*
Pin mappings:
+----------------+--------------+---------------------------------------------+
| Silkscreen pin | Internal pin | Notes                                       |
+----------------+--------------+---------------------------------------------+
| 0              | GPIO0        | ADC1                                        |
| 1              | GPIO1        | ADC1                                        |
| 2              | GPIO2        | ADC1, boot mode / strapping pin             |
| 3              | GPIO3        | ADC1                                        |
| 4              | GPIO4        | ADC1, JTAG                                  |
| 5              | GPIO5        | JTAG                                        |
| 6              | GPIO6        | JTAG                                        |
| 7              | GPIO7        | JTAG                                        |
| 8              | GPIO8        | Blue status_led (inverted), boot mode / strapping pin |
| 9              | GPIO9        | Boot mode / strapping pin, boot button      |
| 10             | GPIO10       |                                             |
| 20             | GPIO20       |                                             |
| 21             | GPIO21       |                                             |
+----------------+--------------+---------------------------------------------+
static const uint8_t TX   = 21;
static const uint8_t RX   = 20;

static const uint8_t SDA  = 8;
static const uint8_t SCL  = 9;

static const uint8_t SS   = 7;
static const uint8_t MOSI = 6;
static const uint8_t MISO = 5;
static const uint8_t SCK  = 4;

static const uint8_t A0   = 0;
static const uint8_t A1   = 1;
static const uint8_t A2   = 2;
static const uint8_t A3   = 3;
static const uint8_t A4   = 4;
static const uint8_t A5   = 5;
*/

/*
  Notes on Pins:
  - The blue status_led is on GPIO8 and is inverted.
  - The Wemos C3 Mini documentation shows an RGB led on GPIO8, but that might be on the C3 Mini only (and not SuperMini).
  - Using status_led toggles a blue on-board LED on the C3 Super Mini.
  - The BOOT button is wired to GPIO9.
  - JTAG is available on GPIO4 - GPIO7.

  Strapping Pins:
  - GPIO2: Floating, controls Boot mode
  - GPIO8: Floating, controls Boot mode, ROM message printing
  - GPIO9: Pull-up, controls Boot mode

  Boot Modes:
  +------------------------+--------+--------+--------+
  | Mode                   | GPIO2  | GPIO8  | GPIO9  |
  +------------------------+--------+--------+--------+
  | SPI boot (default)     | HIGH   | Any    | HIGH   |
  | UART/JTAG download     | HIGH   | HIGH   | LOW    |
  +------------------------+--------+--------+--------+
*/
#define ESP32_RTOS

#define led 8

void setup()
{
  Serial.begin(115200);
  pinMode(led, OUTPUT);
}

// the loop function runs over and over again forever
void loop()
{
  digitalWrite(led, HIGH); // Turn the LED LED white
  float temp_celsius = temperatureRead();
  Serial.print("Temp onBoard ");
  Serial.print(temp_celsius);
  Serial.println("°C");
  delay(2000);
  
  digitalWrite(led, LOW); // Turn the LED LED off
  delay(2000);
}
