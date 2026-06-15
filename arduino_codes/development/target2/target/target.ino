// --- Pin Layout ---
const int greenLEDPin = A0;       // Green LED (+)
const int redLEDPin = A1;         // Red LED (+)
const int photoresistorPin = A2;  // P-Res Signal

// --- Configuration ---
const int thresholdValue = 800;   

void setup() {
  // Initialize pin modes
  pinMode(greenLEDPin, OUTPUT);
  pinMode(redLEDPin, OUTPUT);
  pinMode(photoresistorPin, INPUT); 

  // Turn on the Green LED to indicate the Arduino is powered
  digitalWrite(greenLEDPin, HIGH);

  // Initialize Serial Communication for debugging and calibration
  Serial.begin(9600);
}

void loop() {
  // Read the value from the pertinax coating signal
  int sensorValue = analogRead(photoresistorPin);

  // Print the value to the Serial Monitor so you can see what it reads in real-time
  Serial.print("Sensor Value: ");
  Serial.println(sensorValue);

  // Check if the laser hits the photoresistors (value exceeds threshold)
  if (sensorValue > thresholdValue) {
    digitalWrite(redLEDPin, HIGH); // Turn Red LED ON
  } else {
    digitalWrite(redLEDPin, LOW);  // Turn Red LED OFF
  }

  // A short delay to stabilize the readings
  delay(50);
}