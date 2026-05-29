
import base64
import time

from packages.config import OFFSET_SEND_MAX_HZ
from packages.missions.m1 import Mission1
from packages.missions.m2 import Mission2
from packages.missions.test_detect import TestDetect


class SystemState:
    """Class to manage the state of the system."""

    def __init__(self, link=None, arduino=None):
        self.system_state = "idle"  # Possible states: idle, scanning, tracking, firing
        self.system_mission = "none"  # Possible missions: m1, m2, none
        self.system_data = {}  # Dictionary to hold any relevant data for the missions
        self.link = link
        self.stop_requested = False
        self._last_frame_ts: float = 0.0
        self.frame_target_fps: float = 10.0
        self.frame_jpeg_quality: int = 70
        self.Arduino = arduino
        # Throttle state for current_target_offset Arduino sends.
        self._last_offset_arduino_ts: float = 0.0

    def set_state(self, new_state: str):
        """Sets the current state of the system."""
        valid_states = ["idle", "scanning", "tracking", "firing"]
        if new_state not in valid_states:
            raise ValueError(f"Invalid state: {new_state}. Valid states are: {valid_states}")
        if self.system_state == new_state:
            return
        self.system_state = new_state
        if self.link:
            try:
                self.link.send({"type": "state", "state": new_state})
            except Exception as e:
                print(f"[link send error] {e}")
        # Notify the Arduino so it can drive scanning vs. tracking behavior.
        # Additive — rides the existing data channel, does not touch the
        # current_target_offset wire format.
        if self.Arduino:
            try:
                self.Arduino.send({"type": "data", "key": "system_state", "value": new_state})
            except Exception as e:
                print(f"[arduino send error] {e}")

    def set_mission(self, new_mission: str):
        """Sets the current mission of the system."""
        valid_missions = ["m1", "m2", "td", "none"]
        if new_mission not in valid_missions:
            raise ValueError(f"Invalid mission: {new_mission}. Valid missions are: {valid_missions}")
        self.system_mission = new_mission
        if self.link:
            try:
                self.link.send({"type": "mission", "mission": new_mission})
            except Exception as e:
                print(f"[link send error] {e}")

    def send_data(self, key: str, value):
        """Sends data to the system.

        High-frequency `current_target_offset` is rate-limited to
        OFFSET_SEND_MAX_HZ on the ARDUINO link only (the web link still gets
        every update for the UI). Prevents the Arduino's 64-byte UART RX
        buffer from overflowing and corrupting subsequent state messages.
        """
        self.system_data[key] = value
        if self.link:
            try:
                self.link.send({"type": "data", "key": key, "value": value})
            except Exception as e:
                print(f"[link send error] {e}")
        if self.Arduino:
            if key == "current_target_offset":
                now = time.time()
                if now - self._last_offset_arduino_ts < 1.0 / max(0.1, OFFSET_SEND_MAX_HZ):
                    return  # skip Arduino send; web UI already saw it
                self._last_offset_arduino_ts = now
            try:
                self.Arduino.send({"type": "data", "key": key, "value": value})
            except Exception as e:
                print(f"[arduino send error] {e}")

    def send_frame(self, frame, boxes):
        """JPEG-encodes the frame, attaches detections, sends over link.

        Rate-limited to `frame_target_fps`. `frame` is BGR np.ndarray (cv2),
        `boxes` are detector.BoundingBox objects with xyxy / class_id / confidence.
        """
        if self.link is None or frame is None:
            return
        now = time.time()
        if now - self._last_frame_ts < 1.0 / max(0.1, self.frame_target_fps):
            return
        self._last_frame_ts = now

        # Lazy import to keep cv2 dependency out of state-only callers.
        import cv2 as cv

        ok, buf = cv.imencode(".jpg", frame, [cv.IMWRITE_JPEG_QUALITY, int(self.frame_jpeg_quality)])
        if not ok:
            return
        h, w = frame.shape[:2]
        box_data = []
        for b in boxes or []:
            x1, y1, x2, y2 = b.xyxy
            box_data.append({
                "bbox": [int(x1), int(y1), int(x2 - x1), int(y2 - y1)],
                "cls": int(b.class_id) if b.class_id is not None else None,
                "name": getattr(b, "class_name", None),
                "conf": float(b.confidence) if b.confidence is not None else None,
            })
        try:
            self.link.send({
                "type": "frame",
                "w": int(w),
                "h": int(h),
                "jpeg": base64.b64encode(buf.tobytes()).decode("ascii"),
                "boxes": box_data,
            })
        except Exception as e:
            print(f"[link send frame error] {e}")

    def start_mission(self, mission: str):
        """Starts a specific mission and updates the system state accordingly.
        Catches any mission-level exception so a crash inside a mission cannot
        kill the supervisor process."""

        if self.system_mission != "none":
            print(f"Cannot start mission {mission} because mission {self.system_mission} is already active.")
            return

        self.stop_requested = False
        self.set_mission(mission)
        try:
            if mission == "m1":
                Mission1(self).start()
            elif mission == "m2":
                Mission2(self).start()
            elif mission == "td":
                TestDetect(self).start()
            else:
                self.set_state("idle")
        except Exception as e:
            import traceback
            print(f"[mission {mission} crashed] {e}")
            traceback.print_exc()
        finally:
            self.set_mission("none")
            self.set_state("idle")
            self.stop_requested = False

    def stop_mission(self):
        """Requests the active mission to stop. Mission loop must check `stop_requested`."""
        if self.system_mission == "none":
            print("No active mission to stop.")
            return
        print(f"Stop requested for mission {self.system_mission}.")
        self.stop_requested = True
