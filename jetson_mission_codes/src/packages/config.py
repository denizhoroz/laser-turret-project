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
TARGET_CONFIRM_FRAMES: int = 2
CANDIDATE_MISS_TOLERANCE: int = 5
LOSS_REACQUIRE_FRAMES: int = 2
OFFSET_SEND_MAX_HZ: float = 15.0
TARGET_FIRING_DURATION: float = 3
MOVING_TARGET_TRACKING_DURATION: float = 60.0 
ROI_DWELL_DURATION: float = 1.0   
TCAREA_RATIO: float = 0.6        
PARALLAX_BIAS_Y_PX: int = 60
STUCK_NO_ROI_TIMEOUT: float = 5.0
STUCK_AIMING_TIMEOUT: float = 4.0
FAILSAFE_COOLDOWN: float = 10.0

# Arduino USB serial device
def _detect_arduino_port() -> str:
    if platform.system() == "Windows":
        return "COM8"
    for pattern in ("/dev/ttyUSB0", "/dev/ttyUSB1", "/dev/ttyACM0", "/dev/ttyACM1"):
        if glob.glob(pattern):
            return pattern
    return "/dev/ttyUSB0"

ARDUINO_PORT: str = os.environ.get("ARDUINO_PORT", _detect_arduino_port())
ARDUINO_BAUDRATE: int = int(os.environ.get("ARDUINO_BAUDRATE", "115200"))