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
    TARGET_FIRING_DURATION,
    CANDIDATE_MISS_TOLERANCE,
    LOSS_REACQUIRE_FRAMES,
    STUCK_NO_ROI_TIMEOUT,
    STUCK_AIMING_TIMEOUT,
    FAILSAFE_COOLDOWN,
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
    def _pick_target(boxes, now: float, blacklist: dict[str, float]):
        """Return the box to track this frame.

        Preference: ``target`` class first; ``truck`` only if no target is
        visible. Returns ``None`` if neither class is present. Classes in
        ``blacklist`` with an unexpired cooldown are skipped (expired entries
        are pruned in place).
        """
        target_box = None
        truck_box = None
        for box in boxes:
            name = (box.class_name or "").lower()
            exp = blacklist.get(name)
            if exp is not None:
                if exp > now:
                    continue
                del blacklist[name]
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
        confirm_streak = 0                   # frames a target has been picked during confirmation
        confirm_misses = 0                   # consecutive miss frames during confirmation
        reacq_streak = 0                     # consecutive picks during loss-grace (gates lost_since reset)
        # Failsafe state
        tracking_started_at: float | None = None  # when this lock became active
        roi_first_seen_at: float | None = None    # first time in_roi went True during this lock
        last_picked_name: str | None = None       # class name of most recent picked target (for blacklist)
        failsafe_blacklist: dict[str, float] = {}  # class name -> cooldown expiry ts
        failsafe_fire_until: float = 0.0          # when forced-fire window ends (0 = inactive)

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

            # ---------- Forced-fire window expiry (failsafe-2 cleanup) ----------
            # When the window closes: drop firing, blacklist the class, drop
            # tracking, scan. During the window, normal tracking logic still
            # runs so the motors keep following the moving target — only the
            # in_roi-gated firing decision is overridden (see below).
            if failsafe_fire_until > 0.0 and now >= failsafe_fire_until:
                if firing:
                    firing = False
                    self.system_state.send_data("is_firing", False)
                if last_picked_name:
                    failsafe_blacklist[last_picked_name] = now + FAILSAFE_COOLDOWN
                print(f"[m2] FAILSAFE-2 window ended; blacklisting '{last_picked_name}' & scanning")
                failsafe_fire_until = 0.0
                tracking_active = False
                tracking_started_at = None
                roi_first_seen_at = None
                lost_since = None
                confirm_streak = 0
                confirm_misses = 0
                reacq_streak = 0
                self.system_state.set_state("scanning")
                self.system_state.send_frame(frame, boxes)
                continue

            in_failsafe_fire = failsafe_fire_until > 0.0

            picked = self._pick_target(boxes, now, failsafe_blacklist) if boxes else None
            if picked is not None and picked.class_name:
                last_picked_name = picked.class_name.lower()

            # Confirmation streak (only relevant while not yet tracking). A target
            # must be picked for TARGET_CONFIRM_FRAMES frames before we leave
            # scanning. Brief no-detection frames are absorbed by
            # CANDIDATE_MISS_TOLERANCE so a real but flickery target can still
            # accumulate its streak. Once tracking_active, brief misses are
            # tolerated by the TIMEOUT grace further below.
            if picked is not None:
                confirm_streak += 1
                confirm_misses = 0
            else:
                confirm_misses += 1
                if confirm_misses > CANDIDATE_MISS_TOLERANCE:
                    confirm_streak = 0
                    confirm_misses = 0
            if not tracking_active and confirm_streak >= TARGET_CONFIRM_FRAMES:
                tracking_active = True
                lost_since = None
                confirm_streak = 0
                confirm_misses = 0
                reacq_streak = 0
                tracking_started_at = now
                roi_first_seen_at = None
                print("[m2] target confirmed; tracking active")

            if tracking_active and picked is not None:
                # If we're in loss-grace (lost_since set), require a reacq streak
                # of LOSS_REACQUIRE_FRAMES consecutive picks before resetting the
                # timer. Tentative picks below threshold send NO Arduino traffic,
                # so a single false-positive flicker can't refresh the LED or
                # extend the grace window.
                if lost_since is not None:
                    reacq_streak += 1
                    if reacq_streak >= LOSS_REACQUIRE_FRAMES:
                        lost_since = None
                        reacq_streak = 0
                        print(f"[m2] reacquired {picked.class_name}; resuming tracking")
                    else:
                        print(f"[m2] tentative pick {picked.class_name} (reacq {reacq_streak}/{LOSS_REACQUIRE_FRAMES})")
                        self.system_state.send_frame(frame, boxes)
                        continue

                # Healthy tracking branch.
                self.system_state.set_state("tracking")
                self.offset, self.in_roi = tracker.track(picked)

                # CONTINUOUS tracking — send offset every frame, even in ROI.
                # This is the key behavioral difference from Mission 1.
                self.system_state.send_data("current_target_offset", self.offset)

                # Anchor failsafe-2 timer on first ROI entry for this lock.
                if self.in_roi and roi_first_seen_at is None:
                    roi_first_seen_at = now

                # Failsafe-2 forced fire overrides in_roi gating: motors keep
                # tracking (offsets sent above), laser stays on until window
                # ends. Otherwise normal in_roi-driven firing.
                if in_failsafe_fire:
                    if not firing:
                        firing = True
                        self.system_state.send_data("is_firing", True)
                    self.system_state.set_state("firing")
                elif self.in_roi and not firing:
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

                # Target lost mid forced-fire — kill window now so the next
                # iteration's expiry-cleanup branch fires and blacklists.
                if in_failsafe_fire:
                    failsafe_fire_until = now
                    print("[m2] FAILSAFE-2 cut short — target lost mid-fire")

                # Drop laser immediately on target loss.
                if firing:
                    firing = False
                    self.system_state.send_data("is_firing", False)

                # Reset reacq streak on a miss — must be CONSECUTIVE picks.
                reacq_streak = 0

                if tracking_active:
                    # Was locked on; target missing. Grace before dropping to scan.
                    if lost_since is None:
                        lost_since = now
                        print(f"[m2] target lost; grace starts (TIMEOUT={self.TIMEOUT}s)")
                    elif (now - lost_since) >= self.TIMEOUT:
                        tracking_active = False
                        confirm_streak = 0
                        confirm_misses = 0
                        tracking_started_at = None
                        roi_first_seen_at = None
                        print(f"[m2] loss confirmed (>{self.TIMEOUT}s); resuming scan")
                        self.system_state.set_state("scanning")
                else:
                    # Still scanning (confirming or nothing seen).
                    self.system_state.set_state("scanning")

            # ---------- Failsafes ----------
            # Failsafe 1: locked but crosshair never reaches ROI → false positive
            # heuristic. Drop lock, blacklist the class briefly, return to scan.
            # Failsafe 2: ROI seen but no sustained fire due to flicker → start
            # a forced-fire window (handled at top of loop next iteration).
            if tracking_active and tracking_started_at is not None and failsafe_fire_until == 0.0:
                if roi_first_seen_at is None and (now - tracking_started_at) >= STUCK_NO_ROI_TIMEOUT:
                    bl_name = last_picked_name or ""
                    if bl_name:
                        failsafe_blacklist[bl_name] = now + FAILSAFE_COOLDOWN
                    print(f"[m2] FAILSAFE-1: stuck >{STUCK_NO_ROI_TIMEOUT}s w/o ROI on '{bl_name}' — blacklisting & scanning")
                    if firing:
                        firing = False
                        self.system_state.send_data("is_firing", False)
                    tracking_active = False
                    tracking_started_at = None
                    roi_first_seen_at = None
                    lost_since = None
                    confirm_streak = 0
                    confirm_misses = 0
                    reacq_streak = 0
                    self.system_state.set_state("scanning")
                elif roi_first_seen_at is not None and (now - roi_first_seen_at) >= STUCK_AIMING_TIMEOUT:
                    print(f"[m2] FAILSAFE-2: aimed >{STUCK_AIMING_TIMEOUT}s w/ flicker — forcing fire for {TARGET_FIRING_DURATION}s")
                    if not firing:
                        firing = True
                        self.system_state.send_data("is_firing", True)
                    self.system_state.set_state("firing")
                    failsafe_fire_until = now + TARGET_FIRING_DURATION

            self.system_state.send_frame(frame, boxes)

        # Cleanup: ensure laser flag and state are released before returning
        # to caller (state.py's finally also resets to idle).
        if firing:
            self.system_state.send_data("is_firing", False)
        detector.close_camera()
