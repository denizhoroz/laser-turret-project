"""Jetson side entry point."""

from __future__ import annotations

import logging
import logging.handlers
import os
import threading
import time
import traceback
from pathlib import Path

from packages.link import Link
from packages.state import SystemState
from packages.config import ARDUINO_BAUDRATE, ARDUINO_PORT
from packages.serial import ArduLink


MISSION_MAP = {
    1: "m1", 2: "m2", 3: "td",
    "1": "m1", "2": "m2", "3": "td",
    "m1": "m1", "m2": "m2", "td": "td",
}

SESSION_BACKOFF_S = 2.0
RECV_TIMEOUT_S = 1.0

log = logging.getLogger("jetson")


def setup_logging() -> None:
    fmt = logging.Formatter("%(asctime)s %(levelname)s %(message)s")
    root = logging.getLogger()
    root.setLevel(logging.INFO)

    if not any(isinstance(h, logging.StreamHandler) for h in root.handlers):
        sh = logging.StreamHandler()
        sh.setFormatter(fmt)
        root.addHandler(sh)

    log_dir = Path(__file__).resolve().parent.parent / "logs"
    try:
        log_dir.mkdir(parents=True, exist_ok=True)
        fh = logging.handlers.RotatingFileHandler(
            log_dir / "jetson.log", maxBytes=5 * 1024 * 1024, backupCount=3
        )
        fh.setFormatter(fmt)
        root.addHandler(fh)
    except Exception as e:
        log.warning("could not enable file log: %s", e)


def run_session() -> None:
    """One end-to-end session"""

    host = os.environ.get("GS_HOST", "192.168.55.100")
    port = int(os.environ.get("GS_PORT", "9001"))

    link = Link(host=host, port=port)
    log.info("connecting to ground station at %s:%d ...", host, port)
    link.connect()
    log.info("connected to ground station")

    try:
        arduino = ArduLink(port=ARDUINO_PORT, baudrate=ARDUINO_BAUDRATE)
    except Exception as e:
        log.error("ArduLink open failed (%s); aborting session", e)
        link.close()
        raise

    system_state = SystemState(link=link, arduino=arduino)
    link.send({"type": "hello", "node": "jetson"})
    link.send({"type": "state", "state": system_state.system_state})

    mission_thread: threading.Thread | None = None

    try:
        while link.connected:
            msg = link.recv(timeout=RECV_TIMEOUT_S)
            if msg is None:
                continue
            mtype = msg.get("type")

            if mtype == "mission_start":
                raw = msg.get("mission", msg.get("mission_type"))
                mission = MISSION_MAP.get(raw)
                if mission is None:
                    log.warning("unknown mission: %r", raw)
                    continue
                if mission_thread and mission_thread.is_alive():
                    log.info("mission already running (%s)", system_state.system_mission)
                    continue
                log.info("starting mission %s", mission)
                mission_thread = threading.Thread(
                    target=system_state.start_mission, args=(mission,), daemon=True
                )
                mission_thread.start()

            elif mtype == "mission_stop":
                system_state.stop_mission()

            elif mtype == "ping":
                link.send({"type": "pong"})

            else:
                log.debug("ignoring message: %s", msg)
    finally:
        log.info("session ending; cleanup")
        try:
            system_state.stop_mission()
        except Exception:
            pass
        if mission_thread and mission_thread.is_alive():
            mission_thread.join(timeout=2.0)
        try:
            arduino.close()
        except Exception:
            pass
        try:
            link.close()
        except Exception:
            pass


def main() -> None:
    setup_logging()
    log.info("jetson supervisor starting")
    while True:
        try:
            run_session()
            log.warning("session ended (link down); restarting in %.1fs", SESSION_BACKOFF_S)
        except KeyboardInterrupt:
            log.info("interrupted; exiting")
            return
        except Exception as e:
            log.error("session crashed: %s", e)
            traceback.print_exc()
        time.sleep(SESSION_BACKOFF_S)


if __name__ == "__main__":
    main()
