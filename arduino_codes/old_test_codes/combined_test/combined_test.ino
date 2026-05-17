// --- PIN DEFINITIONS ---
// Dual Stepper Motors
const int stepPin1 = 12; 
const int dirPin1  = 11; 
const int stepPin2 = 10;
const int dirPin2  = 9;

// Trio LEDs
const int greenLED  = A0;
const int yellowLED = A1;
const int redLED    = A2;

// Laser
const int laserPin  = 2;

// --- PARAMETERS ---
const int stepsPerRev = 200; 
const int pulseWidth  = 800; 

void setup() {
  // Motor Setup
  pinMode(stepPin1, OUTPUT);
  pinMode(dirPin1, OUTPUT);
  pinMode(stepPin2, OUTPUT);
  pinMode(dirPin2, OUTPUT);

  // LED Setup
  pinMode(greenLED, OUTPUT);
  pinMode(yellowLED, OUTPUT);
  pinMode(redLED, OUTPUT);

  // Laser Setup
  pinMode(laserPin, OUTPUT);
  
  Serial.begin(9600);
  Serial.println("System Initialized. Starting sequence...");
}

void loop() {
  // --- 1. MOTOR DUAL TEST ---
  Serial.println("Step 1: Motors Spinning...");
  digitalWrite(dirPin1, HIGH);
  digitalWrite(dirPin2, HIGH);

  for(int x = 0; x < stepsPerRev; x++) {
    digitalWrite(stepPin1, HIGH);
    digitalWrite(stepPin2, HIGH);
    delayMicroseconds(pulseWidth); 
    digitalWrite(stepPin1, LOW);
    digitalWrite(stepPin2, LOW);
    delayMicroseconds(pulseWidth);
  }

  delay(1000); 

  digitalWrite(dirPin1, LOW);
  digitalWrite(dirPin2, LOW);

  for(int x = 0; x < stepsPerRev; x++) {
    digitalWrite(stepPin1, HIGH);
    digitalWrite(stepPin2, HIGH);
    delayMicroseconds(pulseWidth); 
    digitalWrite(stepPin1, LOW);
    digitalWrite(stepPin2, LOW);
    delayMicroseconds(pulseWidth);
  }
  delay(1000);

  // --- 2. TRIO LED TEST ---
  Serial.println("Step 2: LED Cycle...");
  digitalWrite(greenLED, HIGH);
  delay(2000);
  digitalWrite(greenLED, LOW);

  digitalWrite(yellowLED, HIGH);
  delay(1000);
  digitalWrite(yellowLED, LOW);

  digitalWrite(redLED, HIGH);
  delay(2000);
  digitalWrite(redLED, LOW);
  delay(1000);

  // --- 3. LASER TEST ---
  Serial.println("Step 3: Laser Pulse...");
  for(int i = 0; i < 3; i++) {
    digitalWrite(laserPin, HIGH);
    delay(500);
    digitalWrite(laserPin, LOW);
    delay(500);
  }
  
  Serial.println("Sequence complete. Restarting in 2 seconds...");
  delay(2000);
}