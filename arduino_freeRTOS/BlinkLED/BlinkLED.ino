/*
  BlinkLED

  Demonstrates usage of onboard LED on some ESP dev boards.
*/
int led = 8;
void setup() {
  pinMode(led,OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(led, HIGH);  // Turn the LED LED white
  delay(1000);
  digitalWrite(led, LOW);  // Turn the LED LED off
  delay(1000);
}
