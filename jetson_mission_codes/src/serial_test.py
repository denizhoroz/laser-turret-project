"""Manual Arduino serial tester.

Sends the same JSON messages main.py would send via SystemState.send_data,
so you can validate motor_serial_test.ino without running a full mission.

Run from src/:
    python serial_test.py

Override port/baud:
    python serial_test.py --port COM5 --baud 115200
"""
from __future__ import annotations

import argparse
import threading
import time

from packages.config import ARDUINO_BAUDRATE, ARDUINO_PORT
from packages.serial import ArduLink


MENU = """
Commands:
  o <x> <y>     send current_target_offset [x, y]    (e.g.  o 10 -5)
  f <0|1>       send is_firing bool                  (e.g.  f 1)
  r             toggle background reader on/off
  s <line>      send raw line as-is (no JSON wrapping)
  q             quit
"""


def reader_loop(arduino: ArduLink, stop: threading.Event):
    while not stop.is_set():
        try:
            msg = arduino.receive()
        except Exception as e:
            print(f"[recv error] {e}")
            time.sleep(0.2)
            continue
        if msg:
            print(f"[arduino] {msg}")


def send_data(arduino: ArduLink, key: str, value):
    payload = {"type": "data", "key": key, "value": value}
    arduino.send(payload)
    print(f"[sent] {payload}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", default=ARDUINO_PORT)
    ap.add_argument("--baud", type=int, default=ARDUINO_BAUDRATE)
    args = ap.parse_args()

    print(f"Opening {args.port} @ {args.baud} ...")
    arduino = ArduLink(port=args.port, baudrate=args.baud)
    print("Open. Letting Arduino reset (2s) ...")
    time.sleep(2.0)

    stop = threading.Event()
    reader_thread: threading.Thread | None = None

    def start_reader():
        nonlocal reader_thread
        if reader_thread and reader_thread.is_alive():
            return
        stop.clear()
        reader_thread = threading.Thread(target=reader_loop, args=(arduino, stop), daemon=True)
        reader_thread.start()
        print("[reader on]")

    def stop_reader():
        stop.set()
        print("[reader off]")

    start_reader()
    print(MENU)

    try:
        while True:
            try:
                line = input("> ").strip()
            except EOFError:
                break
            if not line:
                continue
            parts = line.split()
            cmd = parts[0].lower()

            if cmd == "q":
                break
            elif cmd == "o" and len(parts) == 3:
                try:
                    x, y = int(parts[1]), int(parts[2])
                except ValueError:
                    print("bad ints")
                    continue
                send_data(arduino, "current_target_offset", [x, y])
            elif cmd == "f" and len(parts) == 2:
                send_data(arduino, "is_firing", parts[1] in ("1", "true", "True"))
            elif cmd == "r":
                if reader_thread and reader_thread.is_alive():
                    stop_reader()
                else:
                    start_reader()
            elif cmd == "s" and len(parts) >= 2:
                raw = line[2:]
                arduino.ser.write(raw.encode("utf-8") + b"\n")
                print(f"[sent raw] {raw}")
            else:
                print(MENU)
    finally:
        stop.set()
        arduino.close()
        print("closed.")


if __name__ == "__main__":
    main()
