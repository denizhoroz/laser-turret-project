// MOTOR DUAL TEST

// Pin Definitions
const int stepPin1 = 12; 
const int dirPin1 = 11; 
const int stepPin2 = 10;
const int dirPin2 = 9;
const int laserPin = 2;

// Parameters
const int pulseWidth = 1600;  // Lower = Faster speed
const int rotateAngle = 90;
const int stepsPerRev = 200 / (360 / rotateAngle); // Standard for 1.8 degree motors

void setup() {
  pinMode(stepPin1, OUTPUT);
  pinMode(dirPin1, OUTPUT);
  pinMode(stepPin2, OUTPUT);
  pinMode(dirPin2, OUTPUT);
  pinMode(laserPin, OUTPUT);
}

void loop() {
  // Set direction clockwise
  digitalWrite(dirPin1, HIGH);
  digitalWrite(dirPin2, HIGH);

  // Spin one full revolution
  for(int x = 0; x < stepsPerRev; x++) {
    digitalWrite(stepPin1, HIGH);
    delayMicroseconds(pulseWidth); 
    digitalWrite(stepPin1, LOW);
    delayMicroseconds(pulseWidth);
  }

  delay(1000); // Wait a second
  digitalWrite(laserPin, HIGH);
  delay(1000); // Wait a second
  digitalWrite(laserPin, LOW);
  delay(1000); // Wait a second

  // Spin back one full revolution
  for(int x = 0; x < stepsPerRev; x++) {
    digitalWrite(stepPin2, HIGH);
    delayMicroseconds(pulseWidth); 
    digitalWrite(stepPin2, LOW);
    delayMicroseconds(pulseWidth);
  }

  delay(1000); // Wait a second
  digitalWrite(laserPin, HIGH);
  delay(1000); // Wait a second
  digitalWrite(laserPin, LOW);
  delay(1000); // Wait a second

  // Set direction clockwise
  digitalWrite(dirPin1, LOW);
  digitalWrite(dirPin2, LOW);

  // Spin one full revolution
  for(int x = 0; x < stepsPerRev; x++) {
    digitalWrite(stepPin1, HIGH);
    delayMicroseconds(pulseWidth); 
    digitalWrite(stepPin1, LOW);
    delayMicroseconds(pulseWidth);
  }

  delay(1000); // Wait a second
  digitalWrite(laserPin, HIGH);
  delay(1000); // Wait a second
  digitalWrite(laserPin, LOW);
  delay(1000); // Wait a second

  // Spin back one full revolution
  for(int x = 0; x < stepsPerRev; x++) {
    digitalWrite(stepPin2, HIGH);
    delayMicroseconds(pulseWidth); 
    digitalWrite(stepPin2, LOW);
    delayMicroseconds(pulseWidth);
  }

  delay(1000); // Wait a second
  digitalWrite(laserPin, HIGH);
  delay(1000); // Wait a second
  digitalWrite(laserPin, LOW);
  delay(1000); // Wait a second
}