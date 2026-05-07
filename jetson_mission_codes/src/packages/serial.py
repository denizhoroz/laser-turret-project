import json
import time

import serial

from packages.config import ARDUINO_BAUDRATE, ARDUINO_PORT


class ArduLink:
    """Serial link to Arduino. Reconnects on transient failures."""

    def __init__(self, port=ARDUINO_PORT, baudrate=ARDUINO_BAUDRATE, reset_delay=2.0):
        self.port = port
        self.baudrate = baudrate
        self.reset_delay = reset_delay
        self.ser: serial.Serial | None = None
        self._open()

    def _open(self):
        self.ser = serial.Serial(self.port, self.baudrate, timeout=1)
        time.sleep(self.reset_delay)

    def _reconnect(self) -> bool:
        try:
            if self.ser:
                self.ser.close()
        except Exception:
            pass
        self.ser = None
        try:
            self._open()
            print(f"[arduino] reconnected on {self.port}")
            return True
        except Exception as e:
            print(f"[arduino] reconnect failed: {e}")
            return False

    def send(self, data: dict):
        line = json.dumps(data).encode("utf-8") + b"\n"
        if self.ser is None:
            if not self._reconnect():
                return
        try:
            self.ser.write(line)
        except (serial.SerialException, OSError) as e:
            print(f"[arduino] write failed: {e}; reconnecting")
            if self._reconnect():
                try:
                    self.ser.write(line)
                except Exception as e2:
                    print(f"[arduino] retry write failed: {e2}")

    def receive(self) -> dict:
        if self.ser is None:
            return {}
        try:
            raw = self.ser.readline().decode("utf-8").strip()
        except (serial.SerialException, OSError) as e:
            print(f"[arduino] read failed: {e}")
            return {}
        if not raw:
            return {}
        try:
            return json.loads(raw)
        except json.JSONDecodeError:
            print(f"Received invalid JSON: {raw}")
            return {}

    def close(self):
        try:
            if self.ser:
                self.ser.close()
        except Exception:
            pass
