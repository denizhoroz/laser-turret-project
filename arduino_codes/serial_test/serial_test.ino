// MOTOR SERIAL TEST
//
// Expected incoming messages (newline-terminated, 115200 baud):
//   {"type":"data","key":"current_target_offset","value":[x,y]}
//   {"type":"data","key":"is_firing","value":true}


#include <ArduinoJson.h>

const unsigned long BAUD = 115200;
const size_t JSON_CAP = 256;

int offsetX = 0;
int offsetY = 0;
bool isFiring = false;

void sendAck(const char* status, const char* key) {
  StaticJsonDocument<128> out;
  out["type"] = "ack";
  out["status"] = status;
  if (key) out["key"] = key;
  serializeJson(out, Serial);
  Serial.println();
}

void handleLine(const String& line) {
  StaticJsonDocument<JSON_CAP> doc;
  DeserializationError err = deserializeJson(doc, line);
  if (err) {
    Serial.print("{\"type\":\"error\",\"msg\":\"json_parse\",\"detail\":\"");
    Serial.print(err.c_str());
    Serial.println("\"}");
    return;
  }

  const char* type = doc["type"] | "";
  if (strcmp(type, "data") != 0) {
    sendAck("ignored", nullptr);
    return;
  }

  const char* key = doc["key"] | "";

  if (strcmp(key, "current_target_offset") == 0) {
    JsonArray v = doc["value"].as<JsonArray>();
    if (v.isNull() || v.size() < 2) {
      sendAck("bad_value", key);
      return;
    }
    offsetX = v[0].as<int>();
    offsetY = v[1].as<int>();
    Serial.print("offset x=");
    Serial.print(offsetX);
    Serial.print(" y=");
    Serial.println(offsetY);
    sendAck("ok", key);
    return;
  }

  if (strcmp(key, "is_firing") == 0) {
    isFiring = doc["value"].as<bool>();
    Serial.print("is_firing=");
    Serial.println(isFiring ? "true" : "false");
    sendAck("ok", key);
    return;
  }

  sendAck("unknown_key", key);
}

void setup() {
  Serial.begin(BAUD);
  while (!Serial) { ; } // wait for USB CDC on boards that need it
  Serial.println("{\"type\":\"hello\",\"node\":\"arduino\"}");
}

void loop() {
  static String buf;
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n') {
      buf.trim();
      if (buf.length() > 0) handleLine(buf);
      buf = "";
    } else if (c != '\r') {
      buf += c;
      if (buf.length() > JSON_CAP) buf = ""; // overflow guard
    }
  }
}
