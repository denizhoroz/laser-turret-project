import glob
import os
import platform
from pathlib import Path


WINDOW_SIZE: tuple[int, int] = (800, 600)
BASE_DIR: Path = Path(__file__).resolve().parent
MODELS_DIR: Path = BASE_DIR / "models"
MISSION1_WEIGHTS: Path = MODELS_DIR / "model-target1" / "best.pt"
MISSION2_WEIGHTS: Path = MODELS_DIR / "model-target2" / "best.pt"
DEVICE: str = "cuda"
CAMERA: int = 1
TARGET_LOSS_TIMEOUT: float = 2
# Frames a target must be seen (not necessarily consecutive — see tolerance
# below) before the system commits to tracking it. Filters single/few-frame
# false positives that would otherwise yank the turret out of its scan sweep.
TARGET_CONFIRM_FRAMES: int = 2
# How many no-detection frames can pass during confirmation before the streak
# is wiped. Lets a real target survive brief detection drops while still
# expiring stale candidates that genuinely vanished.
CANDIDATE_MISS_TOLERANCE: int = 5
# While tracking, how many CONSECUTIVE picks are required to consider the
# target reacquired and reset the loss timer. Brief single-frame false
# positives below this threshold are silently ignored (no Arduino traffic).
# Lower = more responsive on real reacquisitions, but flickery false hits can
# stretch the loss-grace and keep the LED lit.
LOSS_REACQUIRE_FRAMES: int = 2
# Max rate (Hz) at which current_target_offset messages are forwarded to the
# Arduino. The Arduino MEGA's hardware UART RX buffer is 64 bytes; each offset
# line is ~55 bytes and applyOffset blocks ~32 ms. Without throttling, bursts
# can fill the buffer and corrupt the *next* incoming line (typically
# system_state, causing scan-state-not-applied / yellow-stuck bugs). Critical
# messages (system_state, is_firing) are NOT throttled — they're edge-triggered.
OFFSET_SEND_MAX_HZ: float = 15.0
TARGET_FIRING_DURATION: float = 3
MOVING_TARGET_TRACKING_DURATION: float = 60.0 # how long to track the moving target 
ROI_DWELL_DURATION: float = 1.0   # target must stay in ROI this long before fire
TCAREA_RATIO: float = 0.6        # TCArea side as fraction of bbox side

# Parallax compensation: camera and laser are mounted apart, so when the
# camera sees the target centered the laser beam still misses. Bias the
# virtual aim point UP by this many pixels (image y+ = down, so a positive
# value shifts the aim above the bbox center). Applied inside Tracker so
# both the commanded offset and the in_roi check use the same biased target.
# Tune by firing at a target at typical engagement distance:
#   laser hits BELOW target → raise the value.
#   laser hits ABOVE target → lower (negative is fine — laser sits above cam).
PARALLAX_BIAS_Y_PX: int = 60

# --- Failsafes for stuck-tracking conditions ---
# (1) STUCK_NO_ROI_TIMEOUT: target locked but the crosshair never reaches its
#     ROI within this many seconds → treat as false positive (e.g. sky/edge
#     ghost), abandon, briefly blacklist the class, resume scan.
# (2) STUCK_AIMING_TIMEOUT: ROI was reached at least once during this lock,
#     but fire never completes within this many seconds (bbox flicker breaks
#     m1 dwell / rapid in-out toggling in m2) → force fire for
#     TARGET_FIRING_DURATION then return to scanning.
STUCK_NO_ROI_TIMEOUT: float = 5.0
STUCK_AIMING_TIMEOUT: float = 4.0
# After a failsafe abandons a class, ignore it for this many seconds so the
# head can sweep off the ghost before the same class becomes re-eligible.
FAILSAFE_COOLDOWN: float = 10.0

# Arduino USB serial device. Env var ARDUINO_PORT wins; otherwise pick a
# platform-appropriate default (Windows: COMx, Linux/Jetson: probe ttyUSB* then ttyACM*).
def _detect_arduino_port() -> str:
    if platform.system() == "Windows":
        return "COM8"
    for pattern in ("/dev/ttyUSB0", "/dev/ttyUSB1", "/dev/ttyACM0", "/dev/ttyACM1"):
        if glob.glob(pattern):
            return pattern
    return "/dev/ttyUSB0"

ARDUINO_PORT: str = os.environ.get("ARDUINO_PORT", _detect_arduino_port())
ARDUINO_BAUDRATE: int = int(os.environ.get("ARDUINO_BAUDRATE", "115200"))