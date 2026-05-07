
import time

from packages.config import (
    WINDOW_SIZE,
    TARGET_LOSS_TIMEOUT,
    TARGET_FIRING_DURATION,
    ROI_DWELL_DURATION,
)

from packages.detector import Detector
from packages.tracker import Tracker


class TestDetect:
    """Stateless detect+track+fire with non-blocking firing window
    and ROI dwell confirmation."""

    def __init__(self, system_state):
        self.system_state = system_state

        self.offset: tuple[int, int] = (0, 0)
        self.in_roi: bool = False
        self.TIMEOUT: float = TARGET_LOSS_TIMEOUT
        self.FIRING_DURATION: float = TARGET_FIRING_DURATION
        self.ROI_DWELL: float = ROI_DWELL_DURATION

    def start(self):
        detector = Detector()
        tracker = Tracker(window_size=WINDOW_SIZE)
        detector.open_camera()

        firing = False
        fire_until = 0.0
        in_roi_since: float | None = None  # epoch when target first entered ROI

        self.system_state.set_state("scanning")
        while not self.system_state.stop_requested:
            res = detector.detect()
            if res is None:
                continue
            frame, boxes = res
            now = time.time()

            # Firing window: stream frames, freeze motors (no offset sent).
            if firing:
                if now >= fire_until:
                    firing = False
                    in_roi_since = None
                    self.in_roi = False
                    self.system_state.send_data("is_firing", False)
                    self.system_state.set_state("scanning")
                self.system_state.send_frame(frame, boxes)
                continue

            # Detect + track
            if boxes:
                box = boxes[0]
                self.system_state.set_state("tracking")
                self.offset, self.in_roi = tracker.track(box)

                # Only command motors when NOT in ROI; once in ROI, freeze
                # so the dwell window can complete without re-overshooting.
                if not self.in_roi:
                    self.system_state.send_data("current_target_offset", self.offset)

                print(f"Offset: {self.offset}, In ROI: {self.in_roi}")
            else:
                self.offset = (0, 0)
                self.in_roi = False
                self.system_state.set_state("scanning")

            # ROI dwell confirmation: must remain in ROI for ROI_DWELL seconds.
            if self.in_roi:
                if in_roi_since is None:
                    in_roi_since = now
                elif (now - in_roi_since) >= self.ROI_DWELL:
                    firing = True
                    fire_until = now + self.FIRING_DURATION
                    self.system_state.set_state("firing")
                    self.system_state.send_data("is_firing", True)
                    print(f"Dwell complete; firing for {self.FIRING_DURATION}s")
            else:
                in_roi_since = None  # reset on any drop-out

            self.system_state.send_frame(frame, boxes)
