
import time

from packages import detector
from packages.config import (
    WINDOW_SIZE,
    TARGET_LOSS_TIMEOUT,
    TARGET_FIRING_DURATION,
    ROI_DWELL_DURATION,
)

from packages.detector import Detector
from packages.tracker import Rectangle, Tracker

class Mission1:
    def __init__(self, system_state):
        self.system_state = system_state

        # Mission specific parameters
        self.target_ids: list[float] = [0, 1, 2, 3, 4]
        self.target_pool: list[float] = [] # Targets that have been shot at least once
        self.selected_target_id: float | None = None
        self.selected_box: Rectangle | None = None
        self.offset: tuple[int, int] = (0, 0)
        self.in_roi: bool = False
        self.TIMEOUT: float = TARGET_LOSS_TIMEOUT
        self.FIRING_DURATION: float = TARGET_FIRING_DURATION
        self.ROI_DWELL: float = ROI_DWELL_DURATION

    def start(self):
        """Starts the mission loop."""

        # Initialize detector and tracker
        detector = Detector()
        tracker = Tracker(window_size=WINDOW_SIZE)
        detector.open_camera()

        # Non-blocking fire window + ROI dwell + target-loss state
        firing = False
        fire_until = 0.0
        in_roi_since: float | None = None
        selected_lost_since: float | None = None  # epoch when selected target first vanished

        self.system_state.set_state("scanning")
        while not self.system_state.stop_requested:
            ## Mission logic ##
            # 1) Start scanning
            # 2) If at least one target detected, check if its not in the target pool
            # 3) If target is in target pool, ignore it and keep scanning
            # 4) If target is not in target pool, select it and start tracking
            # 5) If target is lost for a certain period, keep scanning
            # 6) After tracking for a certain period, add target to target pool and keep scanning
            # 7) If all targets in target pool, finish mission and set system state to idle

            # Get detection results
            res = detector.detect()
            if res is None:
                continue
            frame, boxes = res
            now = time.time()

            # If currently firing, keep streaming frames but skip target updates.
            # Fire window is gated by timestamp so the loop stays live.
            if firing:
                if now >= fire_until:
                    firing = False
                    if self.selected_target_id is not None and self.selected_target_id not in self.target_pool:
                        self.target_pool.append(self.selected_target_id)
                        print(f"Added target ID: {self.selected_target_id} to target pool. Current pool: {self.target_pool}")
                    self.in_roi = False
                    in_roi_since = None
                    self.selected_target_id = None
                    self.selected_box = None
                    self.system_state.send_data("is_firing", False)
                    self.system_state.set_state("scanning")
                self.system_state.send_frame(frame, boxes)
                continue

            # Calculate offsets and check if targets are in ROI
            if boxes:
                seen_selected = False
                for box in boxes:
                    target_id = box.class_id if box.class_id is not None else -1

                    # If no target is currently selected and the detected target is not in the target pool, select it and start tracking
                    if self.selected_target_id is None and target_id not in self.target_pool:
                        print(f"Selected target ID: {target_id} for tracking.")
                        self.selected_target_id = target_id
                        self.system_state.set_state("tracking")

                    # If the detected target is the currently selected target, continue tracking it
                    if target_id == self.selected_target_id:
                        seen_selected = True
                        self.offset, self.in_roi = tracker.track(box)

                        # Freeze motors once in ROI so dwell window can complete.
                        if not self.in_roi:
                            self.system_state.send_data("current_target_offset", self.offset)

                        print(f"Offset: {self.offset}, In ROI: {self.in_roi}, Tracking target ID: {self.selected_target_id}")

                # Selected target lost: start grace timer; drop only after TIMEOUT.
                if self.selected_target_id is not None and not seen_selected:
                    if selected_lost_since is None:
                        selected_lost_since = now
                    elif (now - selected_lost_since) >= self.TIMEOUT:
                        print(f"Selected target ID: {self.selected_target_id} lost > {self.TIMEOUT}s; clearing.")
                        self.selected_target_id = None
                        self.selected_box = None
                        self.in_roi = False
                        selected_lost_since = None
                        self.system_state.set_state("scanning")
                else:
                    selected_lost_since = None  # target reacquired (or none selected)
            else:
                self.offset = (0, 0)
                self.in_roi = False
                if self.selected_target_id is not None:
                    if selected_lost_since is None:
                        selected_lost_since = now
                    elif (now - selected_lost_since) >= self.TIMEOUT:
                        print(f"Selected target ID: {self.selected_target_id} lost > {self.TIMEOUT}s; clearing.")
                        self.selected_target_id = None
                        self.selected_box = None
                        selected_lost_since = None
                self.system_state.set_state("scanning")

            # ROI dwell confirmation: must remain in ROI for ROI_DWELL seconds before fire.
            if self.in_roi:
                if in_roi_since is None:
                    in_roi_since = now
                elif (now - in_roi_since) >= self.ROI_DWELL:
                    firing = True
                    fire_until = now + self.FIRING_DURATION
                    self.system_state.set_state("firing")
                    self.system_state.send_data("is_firing", True)
                    print(f"Dwell complete; firing at target ID: {self.selected_target_id} for {self.FIRING_DURATION}s")
            else:
                in_roi_since = None  # reset on drop-out

            self.system_state.send_frame(frame, boxes)
            # detector.display(frame, boxes, tracker=tracker, selected_id=self.selected_target_id, offset=self.offset) # local cv window (disabled — viewing on web)

    def is_target_in_pool(self, target_id):
        """Checks if the target ID is in the target pool."""
        return True if target_id in self.target_pool else False
