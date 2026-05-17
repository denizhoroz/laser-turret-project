// MOTOR PITCH TEST

// Pin Definitions
const int stepPin2 = 10;
const int dirPin2 = 9;

// Parameters

const int pulseWidth = 2000;  // Lower = Faster speed
const int rotateAngle = 90;
const int stepsPerRev = 200 / (360 / rotateAngle); // Standard for 1.8 degree motors

/*
stepsPerRev = 200 = 360 degrees
stepsPerRev = 200/4 = 360/4 degrees
stepsPerRev = 50 = 90 degrees
*/

void setup() {
  pinMode(stepPin2, OUTPUT);
  pinMode(dirPin2, OUTPUT);
}

void loop() {
  // Set direction clockwise
  digitalWrite(dirPin2, HIGH);

  // Spin one full revolution
  for(int x = 0; x < stepsPerRev; x++) {
    digitalWrite(stepPin2, HIGH);
    delayMicroseconds(pulseWidth); 
    digitalWrite(stepPin2, LOW);
    delayMicroseconds(pulseWidth);
  }

  delay(1000); // Wait a second

  // Set direction counter-clockwise
  digitalWrite(dirPin2, LOW);

  // Spin back one full revolution
  for(int x = 0; x < stepsPerRev; x++) {
    digitalWrite(stepPin2, HIGH);
    delayMicroseconds(pulseWidth); 
    digitalWrite(stepPin2, LOW);
    delayMicroseconds(pulseWidth);
  }

  delay(1000);
}