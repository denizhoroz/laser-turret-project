// TRIO LED TEST

// Pin Definitions
const int greenLED  = A0;
const int yellowLED = A1;
const int redLED    = A2;

void setup() {
  // Initialize pins as outputs
  pinMode(greenLED, OUTPUT);
  pinMode(yellowLED, OUTPUT);
  pinMode(redLED, OUTPUT);
}

void loop() {
  // Green ON
  digitalWrite(greenLED, HIGH);
  delay(2000);
  digitalWrite(greenLED, LOW);

  // Yellow ON
  digitalWrite(yellowLED, HIGH);
  delay(1000);
  digitalWrite(yellowLED, LOW);

  // Red ON
  digitalWrite(redLED, HIGH);
  delay(2000);
  digitalWrite(redLED, LOW);
}