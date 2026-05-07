"""Jetson side entry point.

Connects to ground station over Link (TCP for dev, swap to serial in prod).
Listens for mission_start / mission_stop commands and runs the requested
mission in a worker thread, while the main thread keeps draining commands.
"""
from __future__ import annotations

import os
import threading

from packages.link import Link
from packages.state import SystemState
from packages.config import ARDUINO_BAUDRATE, ARDUINO_PORT
from packages.serial import ArduLink


MISSION_MAP = {
    1: "m1", 2: "m2", 3: "td",
    "1": "m1", "2": "m2", "3": "td",
    "m1": "m1", "m2": "m2", "td": "td",
}


def main():
    # Connecting to the ground station
    host = os.environ.get("GS_HOST", "127.0.0.1")
    port = int(os.environ.get("GS_PORT", "9001"))

    link = Link(host=host, port=port)
    print(f"[jetson] connecting to ground station at {host}:{port} ...")
    link.connect()
    print("[jetson] connected")

    # Connecting to the Arduino and initializing system state
    arduino = ArduLink(port=ARDUINO_PORT, baudrate=ARDUINO_BAUDRATE)

    system_state = SystemState(link=link, arduino=arduino)
    link.send({"type": "hello", "node": "jetson"})
    link.send({"type": "state", "state": system_state.system_state})

    mission_thread: threading.Thread | None = None

    while True:
        msg = link.recv(timeout=1.0)
        if msg is None:
            continue
        mtype = msg.get("type")

        if mtype == "mission_start":
            raw = msg.get("mission", msg.get("mission_type"))
            mission = MISSION_MAP.get(raw)
            if mission is None:
                print(f"[jetson] unknown mission: {raw!r}")
                continue
            if mission_thread and mission_thread.is_alive():
                print(f"[jetson] mission already running ({system_state.system_mission})")
                continue
            print(f"[jetson] starting mission {mission}")
            mission_thread = threading.Thread(
                target=system_state.start_mission, args=(mission,), daemon=True
            )
            mission_thread.start()

        elif mtype == "mission_stop":
            system_state.stop_mission()

        elif mtype == "ping":
            link.send({"type": "pong"})

        else:
            print(f"[jetson] ignoring message: {msg}")


if __name__ == "__main__":
    main()
