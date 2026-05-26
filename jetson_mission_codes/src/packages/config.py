from pathlib import Path


WINDOW_SIZE: tuple[int, int] = (640, 480)
BASE_DIR: Path = Path(__file__).resolve().parent
MODELS_DIR: Path = BASE_DIR / "models"
MISSION1_WEIGHTS: Path = MODELS_DIR / "model-target1" / "best.pt"
MISSION2_WEIGHTS: Path = MODELS_DIR / "model-target2" / "best.pt"
DEVICE: str = "cuda"
CAMERA: int = 1
TARGET_LOSS_TIMEOUT: float = 2
TARGET_FIRING_DURATION: float = 3
ROI_DWELL_DURATION: float = 1.0   # target must stay in ROI this long before fire
TCAREA_RATIO: float = 0.4         # TCArea side as fraction of bbox side
ARDUINO_PORT: str = "COM8" 
ARDUINO_BAUDRATE: int = 115200