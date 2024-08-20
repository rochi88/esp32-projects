# Readme

## Parts Required
To follow this tutorial you need the following parts:

- ESP32 (read Best ESP32 development boards)
- DS18B20 temperature sensor (one or multiple sensors) – waterproof version
- 4.7k Ohm resistor
- Jumper wires
- Breadboard

## Installing Libraries
To interface with the DS18B20 temperature sensor, you need to install the One Wire library by Paul Stoffregen and the Dallas Temperature library. Follow the next steps to install those libraries.

1. Open your Arduino IDE and go to Sketch > Include Library > Manage Libraries. The Library Manager should open.

2. Type “onewire” in the search box and install OneWire library by Paul Stoffregen.

3. Then, search for “Dallas” and install DallasTemperature library by Miles Burton.
