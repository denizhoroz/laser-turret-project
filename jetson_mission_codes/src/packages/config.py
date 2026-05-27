from pathlib import Path


WINDOW_SIZE: tuple[int, int] = (640, 480)
BASE_DIR: Path = Path(__file__).resolve().parent
MODELS_DIR: Path = BASE_DIR / "models"
MISSION1_WEIGHTS: Path = MODELS_DIR / "model-target1" / "best.pt"
MISSION2_WEIGHTS: Path = MODELS_DIR / "model-target2" / "best.pt"
DEVICE: str = "cuda"
CAMERA: int = 1
TARGET_LOSS_TIMEOUT: float = 2
# Consecutive frames a target must be detected before the system commits to
# tracking it (and leaves scanning). Filters single/few-frame false positives
# (sky/ground noise) that would otherwise yank the turret out of its scan sweep.
TARGET_CONFIRM_FRAMES: int = 2
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
PARALLAX_BIAS_Y_PX: int = 50

ARDUINO_PORT: str = "COM8"
ARDUINO_BAUDRATE: int = 115200