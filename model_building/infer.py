import argparse
import time
from pathlib import Path

import cv2
import torch
from ultralytics import YOLO

BASE_DIR = Path(__file__).resolve().parent
DEFAULT_WEIGHTS = BASE_DIR / "model_train" / "runs" / "yolo26s_shapes" / "weights" / "best.pt"


def parse_source(src: str):
    if src.isdigit():
        return int(src)
    return src


def main():
    ap = argparse.ArgumentParser(description="YOLO26s video inference")
    ap.add_argument("--weights", type=str, default=str(DEFAULT_WEIGHTS))
    ap.add_argument("--source", type=str, default="0",
                    help="webcam index (e.g. 0) or video file path")
    ap.add_argument("--imgsz", type=int, default=640)
    ap.add_argument("--conf", type=float, default=0.25)
    ap.add_argument("--iou", type=float, default=0.45)
    ap.add_argument("--device", type=str,
                    default="cuda" if torch.cuda.is_available() else "cpu")
    args = ap.parse_args()

    weights = Path(args.weights)
    assert weights.exists(), f"weights not found: {weights}"

    model = YOLO(str(weights))
    print(f"loaded {weights.name} on {args.device}")

    cap = cv2.VideoCapture(parse_source(args.source))
    assert cap.isOpened(), f"cannot open source: {args.source}"

    win = "YOLO26s inference - q/ESC to quit"
    cv2.namedWindow(win, cv2.WINDOW_NORMAL)

    prev = time.time()
    fps = 0.0
    try:
        while True:
            ok, frame = cap.read()
            if not ok:
                print("stream end")
                break

            results = model.predict(
                frame,
                imgsz=args.imgsz,
                conf=args.conf,
                iou=args.iou,
                device=args.device,
                verbose=False,
            )
            annotated = results[0].plot()

            now = time.time()
            dt = now - prev
            prev = now
            if dt > 0:
                fps = 0.9 * fps + 0.1 * (1.0 / dt) if fps else 1.0 / dt
            cv2.putText(annotated, f"FPS: {fps:.1f}", (10, 30),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2)

            cv2.imshow(win, annotated)
            k = cv2.waitKey(1) & 0xFF
            if k in (ord("q"), 27):
                break
    finally:
        cap.release()
        cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
