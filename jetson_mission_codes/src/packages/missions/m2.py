"""Mission 2 — live-fire tracking of a moving target.

Two YOLO classes exist in the m2 model: ``target`` (preferred) and ``truck``
(fallback).

Behavior:
    1. Scan for any detection.
    2. On detection, pick a target by class preference: ``target`` wins over
       ``truck``. Switch to ``tracking`` state.
    3. While the target is visible, *continuously* command the motors via
       ``current_target_offset``. Unlike Mission 1 there is no in-ROI motor
       freeze — we trace the moving car.
    4. When the crosshair enters the bounding box's ROI, raise
       ``is_firing=True``. When it leaves the ROI, lower it. Laser tracks
       ROI state directly — no dwell, no stop-to-aim.
    5. Lost target for more than ``TARGET_LOSS_TIMEOUT`` seconds → back to
       scanning. Mission caps at ``MOVING_TARGET_TRACKING_DURATION`` seconds.
"""

import time

from packages.config import (
    WINDOW_SIZE,
    MISSION2_WEIGHTS,
    TARGET_LOSS_TIMEOUT,
    TARGET_CONFIRM_FRAMES,
)
from packages.detector import Detector
from packages.tracker import Tracker


class Mission2:
    def __init__(self, system_state):
        self.system_state = system_state

        # Per-frame state
        self.offset: tuple[int, int] = (0, 0)
        self.in_roi: bool = False

        # Tunables from config
        self.TIMEOUT: float = TARGET_LOSS_TIMEOUT

    @staticmethod
    def _pick_target(boxes):
        """Return the box to track this frame.

        Preference: ``target`` class first; ``truck`` only if no target is
        visible. Returns ``None`` if neither class is present.
        """
        target_box = None
        truck_box = None
        for box in boxes:
            name = (box.class_name or "").lower()
            if name == "target" and target_box is None:
                target_box = box
            elif name == "truck" and truck_box is None:
                truck_box = box
        return target_box or truck_box

    def start(self):
        detector = Detector(weights=str(MISSION2_WEIGHTS))
        tracker = Tracker(window_size=WINDOW_SIZE)
        detector.open_camera()

        firing = False                       # local mirror of is_firing sent to Arduino
        lost_since: float | None = None      # epoch when target first vanished (while tracking)
        tracking_active = False              # False = scanning, True = locked on a target
        confirm_streak = 0                   # consecutive frames a target has been picked

        self.system_state.set_state("scanning")

        while not self.system_state.stop_requested:
            # NOTE: mission timer (MOVING_TARGET_TRACKING_DURATION) temporarily
            # disabled. Mission runs until stop_requested. Restore by adding
            # back the elapsed-time check here.

            res = detector.detect()
            if res is None:
                continue
            frame, boxes = res
            now = time.time()

            picked = self._pick_target(boxes) if boxes else None

            # Confirmation streak: a target must be picked for TARGET_CONFIRM_FRAMES
            # consecutive frames before we leave scanning. Once tracking, brief
            # misses are tolerated by the TIMEOUT grace below (hysteresis), so a
            # 1-frame false positive can't yank us out of the scan sweep.
            confirm_streak = confirm_streak + 1 if picked is not None else 0
            if not tracking_active and confirm_streak >= TARGET_CONFIRM_FRAMES:
                tracking_active = True
                lost_since = None

            if tracking_active and picked is not None:
                lost_since = None
                self.system_state.set_state("tracking")

                self.offset, self.in_roi = tracker.track(picked)

                # CONTINUOUS tracking — send offset every frame, even in ROI.
                # This is the key behavioral difference from Mission 1.
                self.system_state.send_data("current_target_offset", self.offset)

                # Laser follows ROI state directly. Edge-triggered sends so we
                # don't spam Arduino with redundant is_firing messages.
                if self.in_roi and not firing:
                    firing = True
                    self.system_state.send_data("is_firing", True)
                    self.system_state.set_state("firing")
                    print(f"[m2] in ROI; firing on {picked.class_name}")
                elif not self.in_roi and firing:
                    firing = False
                    self.system_state.send_data("is_firing", False)
                    self.system_state.set_state("tracking")
                    print(f"[m2] out of ROI; ceasing fire on {picked.class_name}")

                print(
                    f"[m2] cls={picked.class_name} offset={self.offset} "
                    f"in_roi={self.in_roi} firing={firing}"
                )
            else:
                # Either scanning (target not yet confirmed) or tracking with the
                # target missing this frame.
                self.offset = (0, 0)
                self.in_roi = False

                # Drop laser immediately on target loss.
                if firing:
                    firing = False
                    self.system_state.send_data("is_firing", False)

                if tracking_active:
                    # Was locked on; target missing. Grace before dropping to scan.
                    if lost_since is None:
                        lost_since = now
                    elif (now - lost_since) >= self.TIMEOUT:
                        tracking_active = False
                        confirm_streak = 0
                        self.system_state.set_state("scanning")
                else:
                    # Still scanning (confirming or nothing seen).
                    self.system_state.set_state("scanning")

            self.system_state.send_frame(frame, boxes)

        # Cleanup: ensure laser flag and state are released before returning
        # to caller (state.py's finally also resets to idle).
        if firing:
            self.system_state.send_data("is_firing", False)
        detector.close_camera()
