// MOTOR DUAL TEST

// Pin Definitions
const int stepPin1 = 12; 
const int dirPin1 = 11; 
const int stepPin2 = 10;
const int dirPin2 = 9;

// Parameters
const int stepsPerRev = 200; // Standard for 1.8 degree motors
const int pulseWidth = 800;  // Lower = Faster speed

void setup() {
  pinMode(stepPin1, OUTPUT);
  pinMode(dirPin1, OUTPUT);
  pinMode(stepPin2, OUTPUT);
  pinMode(dirPin2, OUTPUT);
}

void loop() {
  // Set direction clockwise
  digitalWrite(dirPin1, HIGH);
  digitalWrite(dirPin2, HIGH);

  // Spin one full revolution
  for(int x = 0; x < stepsPerRev; x++) {
    digitalWrite(stepPin1, HIGH);
    digitalWrite(stepPin2, HIGH);
    delayMicroseconds(pulseWidth); 
    digitalWrite(stepPin1, LOW);
    digitalWrite(stepPin2, LOW);
    delayMicroseconds(pulseWidth);
  }

  delay(1000); // Wait a second

  // Set direction counter-clockwise
  digitalWrite(dirPin1, LOW);
  digitalWrite(dirPin2, LOW);

  // Spin back one full revolution
  for(int x = 0; x < stepsPerRev; x++) {
    digitalWrite(stepPin1, HIGH);
    digitalWrite(stepPin2, HIGH);
    delayMicroseconds(pulseWidth); 
    digitalWrite(stepPin1, LOW);
    digitalWrite(stepPin2, LOW);
    delayMicroseconds(pulseWidth);
  }

  delay(1000);
}