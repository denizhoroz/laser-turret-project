// Limit Switch Test - 4 switches, debounced
// Wiring: C -> A12/A13/A14/A15, NC -> GND

const unsigned long DEBOUNCE_MS = 50;

bool stable1, stable2, stable3, stable4;
bool reading1, reading2, reading3, reading4;
unsigned long change1, change2, change3, change4;

void setup() {
  Serial.begin(115200);

  pinMode(A12, INPUT_PULLUP);
  pinMode(A13, INPUT_PULLUP);
  pinMode(A14, INPUT_PULLUP);
  pinMode(A15, INPUT_PULLUP);

  stable1 = reading1 = digitalRead(A12);
  stable2 = reading2 = digitalRead(A13);
  stable3 = reading3 = digitalRead(A14);
  stable4 = reading4 = digitalRead(A15);

  Serial.println("Limit switch test ready. NC->GND, pull-ups active, 50ms debounce.");
  Serial.println("SW1 -> A12");
  Serial.println("SW2 -> A13");
  Serial.println("SW3 -> A14");
  Serial.println("SW4 -> A15");
}

void loop() {
  unsigned long now = millis();

  bool r1 = digitalRead(A12);
  if (r1 != reading1) { reading1 = r1; change1 = now; }
  if (now - change1 >= DEBOUNCE_MS && r1 != stable1) {
    stable1 = r1;
    Serial.print("SW1 (A12): ");
    Serial.println(stable1 == LOW ? "OPEN" : "TRIGGERED");
  }

  bool r2 = digitalRead(A13);
  if (r2 != reading2) { reading2 = r2; change2 = now; }
  if (now - change2 >= DEBOUNCE_MS && r2 != stable2) {
    stable2 = r2;
    Serial.print("SW2 (A13): ");
    Serial.println(stable2 == LOW ? "OPEN" : "TRIGGERED");
  }

  bool r3 = digitalRead(A14);
  if (r3 != reading3) { reading3 = r3; change3 = now; }
  if (now - change3 >= DEBOUNCE_MS && r3 != stable3) {
    stable3 = r3;
    Serial.print("SW3 (A14): ");
    Serial.println(stable3 == LOW ? "OPEN" : "TRIGGERED");
  }

  bool r4 = digitalRead(A15);
  if (r4 != reading4) { reading4 = r4; change4 = now; }
  if (now - change4 >= DEBOUNCE_MS && r4 != stable4) {
    stable4 = r4;
    Serial.print("SW4 (A15): ");
    Serial.println(stable4 == LOW ? "OPEN" : "TRIGGERED");
  }
}