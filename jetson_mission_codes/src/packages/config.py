from pathlib import Path


WINDOW_SIZE: tuple[int, int] = (640, 480)
BASE_DIR: Path = Path(__file__).resolve().parent
DEFAULT_WEIGHTS: Path = BASE_DIR / "models" / "best.pt"
DEVICE: str = "cuda"
CAMERA: int = 0
TARGET_LOSS_TIMEOUT: float = 2
TARGET_FIRING_DURATION: float = 3