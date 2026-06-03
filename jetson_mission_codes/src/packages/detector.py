import time
from pathlib import Path

import cv2 as cv
import ultralytics

# Parameters
from packages import tracker
from packages.config import DEVICE, CAMERA, WINDOW_SIZE

# Min seconds between camera-reopen attempts when the device is down.
# Hammering VideoCapture() too fast on Windows MSMF can hang for several
# hundred ms each call, starving the mission loop.
_RECONNECT_INTERVAL_S = 1.0

class BoundingBox:
    """Represents a bounding box with coordinates, class ID, name, and confidence."""
    def __init__(self, xyxy, class_id=None, confidence=None, class_name=None):
        self.xyxy = xyxy  # [x1, y1, x2, y2]
        self.class_id = class_id
        self.confidence = confidence
        self.class_name = class_name

class Detector:
    """YOLO26s object detector for video stream inference.
    
    Args:
        weights (str): Path to the YOLO26s model weights.
        device (str): Inference device, e.g. "cuda" or "cpu".
        camera (int): Webcam index for video capture."""

    def __init__(self,
                 weights: str,
                 device: str = DEVICE,
                 camera: int = CAMERA,
                 **kwargs):

        weights_path = Path(weights)
        assert weights_path.exists(), f"weights not found: {weights_path}"

        self.model = ultralytics.YOLO(str(weights_path))
        assert self.model is not None, f"failed to load model from {weights_path}"

        self.device = device
        self.camera = camera
        assert isinstance(self.camera, int), "camera index must be an integer"

        self.conf = kwargs.get("conf", 0.50)
        self.iou = kwargs.get("iou", 0.45)

        self._camera_down: bool = False
        self._last_reconnect_attempt: float = 0.0

    def open_camera(self):
        """Opens the video capture for the specified camera index."""
        self.cap = cv.VideoCapture(self.camera)
        assert self.cap.isOpened(), f"cannot open camera index {self.camera}"
        self._apply_capture_size(self.cap)
        self._camera_down = False

    def _apply_capture_size(self, cap):
        w, h = WINDOW_SIZE
        cap.set(cv.CAP_PROP_FRAME_WIDTH, int(w))
        cap.set(cv.CAP_PROP_FRAME_HEIGHT, int(h))
        actual_w = int(cap.get(cv.CAP_PROP_FRAME_WIDTH))
        actual_h = int(cap.get(cv.CAP_PROP_FRAME_HEIGHT))
        if (actual_w, actual_h) != (int(w), int(h)):
            print(f"warning: requested {w}x{h}, camera delivers {actual_w}x{actual_h}")

    def close_camera(self):
        """Releases the video capture resource."""
        if hasattr(self, "cap") and self.cap.isOpened():
            self.cap.release()
            cv.destroyAllWindows()

    def _try_reconnect(self) -> bool:
        """Rate-limited camera reopen. Returns True on success."""
        now = time.monotonic()
        if now - self._last_reconnect_attempt < _RECONNECT_INTERVAL_S:
            return False
        self._last_reconnect_attempt = now
        try:
            if hasattr(self, "cap"):
                self.cap.release()
        except Exception:
            pass
        cap = cv.VideoCapture(self.camera)
        if cap.isOpened():
            self._apply_capture_size(cap)
            self.cap = cap
            self._camera_down = False
            print("camera reconnected")
            return True
        return False

    def detect(self):
        """Runs a single object detection pass and returns the frame with bounding boxes.

        Returns:
            tuple: (frame, boxes)
                frame (np.ndarray): Captured image from the camera.
                boxes (list[dict]): Detected bounding boxes and metadata.
        """

        if self._camera_down:
            self._try_reconnect()
            return None

        try:
            ret, frame = self.cap.read()
        except Exception as e:
            print(f"camera read raised: {e} — will retry")
            self._camera_down = True
            return None

        if not ret or frame is None:
            if not self._camera_down:
                print("camera disconnected — will retry until reconnected")
            self._camera_down = True
            return None

        results = self.model.predict(frame,
                                        device=self.device,
                                        conf=self.conf,
                                        iou=self.iou,
                                        verbose=False)

        # One detection per frame per class: if a class appears more than once,
        # keep only its highest-confidence box. Stops the tracker from flipping
        # between two same-class boxes (e.g. two circles) in the same frame.
        best_per_class = {}
        if len(results) > 0 and len(results[0].boxes) > 0:
            xyxy = results[0].boxes.xyxy.tolist()
            cls_ids = results[0].boxes.cls.tolist() if results[0].boxes.cls is not None else []
            confs = results[0].boxes.conf.tolist() if results[0].boxes.conf is not None else []

            names_map = self.model.names if hasattr(self.model, "names") else {}
            for idx, coords in enumerate(xyxy):
                cid = cls_ids[idx] if idx < len(cls_ids) else None
                cname = None
                if cid is not None:
                    try:
                        cname = names_map[int(cid)]
                    except (KeyError, IndexError, TypeError):
                        cname = None
                box = BoundingBox(xyxy=coords,
                                  class_id=cid,
                                  confidence=confs[idx] if idx < len(confs) else None,
                                  class_name=cname)
                existing = best_per_class.get(cid)
                if existing is None or (box.confidence or 0) > (existing.confidence or 0):
                    best_per_class[cid] = box

        boxes = list(best_per_class.values())
        return frame, boxes
    
    def display(self, frame, boxes, **kwargs):
        """Displays the frame with detected bounding boxes.

        Args:
            frame (np.ndarray): The image frame to display.
            boxes (list[dict]): List of detected bounding boxes and metadata.
        """

        # Optionally draw the center if provided
        tracker = kwargs.get("tracker", None)
        if tracker is not None:
            # Draw the center point
            cv.circle(frame, tracker.center, 5, (255, 0, 0), -1)   

        for box in boxes:
            x1, y1, x2, y2 = map(int, box.xyxy)
            class_id = box.class_id if box.class_id is not None else -1
            class_name = self.model.names[class_id] if class_id >= 0 and class_id < len(self.model.names) else "unknown"
            confidence = box.confidence if box.confidence is not None else 0

            label = f"{class_name} {confidence:.2f}" if class_id >= 0 else f"{confidence:.2f}"
            cv.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
            cv.putText(frame, label, (x1, y1 - 10), cv.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)

            selected_id = kwargs.get("selected_id", None)
            if box.class_id == selected_id:
                # Draw a rectangle of TCArea if tracking
                cv.rectangle(frame, 
                            (tracker.TCArea.x, tracker.TCArea.y), 
                            (tracker.TCArea.x + tracker.TCArea.width, tracker.TCArea.y + tracker.TCArea.height), 
                            (0, 255, 255), 2)

                # Draw a line from the center of the screen to the center of the boxes if tracking
                target_center = ((x1 + x2) // 2, (y1 + y2) // 2)
                cv.line(frame, tracker.center, target_center, (0, 255, 255), 2)

                # Write the offset from the center of the screen to the center of the box on the center of the frame if tracking
                offset = kwargs.get("offset", (0, 0))
                offset_text = f"Offset: ({offset[0]}, {offset[1]})"
                cv.putText(frame, offset_text, ((frame.shape[1] // 2 - 60), (frame.shape[0] // 2) - 20), cv.FONT_HERSHEY_SIMPLEX, 0.4, (0, 255, 255), 2)
                
        cv.imshow("YOLO26s Detection", frame)
        cv.waitKey(1)


    def test_camera(self):
        """Test method to verify camera capture and display functionality."""
        res, frame = self.cap.read()
        if not res:
            print("failed to read frame from camera")
            return
        cv.imshow("Camera Test", frame)
        k = cv.waitKey(1) & 0xFF
        if k in (ord("q"), 27):
            self.close_camera()