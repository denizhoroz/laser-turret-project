// LASER TEST

// Pin Definition
const int laserPin = 2;

void setup() {
  // Set the laser pin as an output
  pinMode(laserPin, OUTPUT);
}

void loop() {
  digitalWrite(laserPin, HIGH); // Turn laser ON
  delay(1000);                  // Wait 1 second
  
  digitalWrite(laserPin, LOW);  // Turn laser OFF
  delay(1000);                  // Wait 1 second
}