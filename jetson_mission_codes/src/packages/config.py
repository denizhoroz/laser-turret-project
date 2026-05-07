from pathlib import Path


WINDOW_SIZE: tuple[int, int] = (640, 480)
BASE_DIR: Path = Path(__file__).resolve().parent
DEFAULT_WEIGHTS: Path = BASE_DIR / "models" / "best.pt"
DEVICE: str = "cuda"
CAMERA: int = 0
TARGET_LOSS_TIMEOUT: float = 2
TARGET_FIRING_DURATION: float = 3
ROI_DWELL_DURATION: float = 1.0   # target must stay in ROI this long before fire
TCAREA_RATIO: float = 0.4         # TCArea side as fraction of bbox side
ARDUINO_PORT: str = "COM9" 
ARDUINO_BAUDRATE: int = 115200