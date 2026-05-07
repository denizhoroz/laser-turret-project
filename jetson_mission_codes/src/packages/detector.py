from pathlib import Path

import cv2 as cv
import ultralytics

# Parameters
from packages import tracker
from packages.config import DEFAULT_WEIGHTS, DEVICE, CAMERA

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
                 weights: str = str(DEFAULT_WEIGHTS), 
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

    def open_camera(self):
        """Opens the video capture for the specified camera index."""
        self.cap = cv.VideoCapture(self.camera)
        assert self.cap.isOpened(), f"cannot open camera index {self.camera}"

    def close_camera(self):
        """Releases the video capture resource."""
        if hasattr(self, "cap") and self.cap.isOpened():
            self.cap.release()
            cv.destroyAllWindows()

    def detect(self):
        """Runs a single object detection pass and returns the frame with bounding boxes.

        Returns:
            tuple: (frame, boxes)
                frame (np.ndarray): Captured image from the camera.
                boxes (list[dict]): Detected bounding boxes and metadata.
        """

        ret, frame = self.cap.read()
        if not ret:
            print("failed to read frame from camera")
            return None, []

        results = self.model.predict(frame,
                                        device=self.device,
                                        conf=self.conf,
                                        iou=self.iou,
                                        verbose=False)

        boxes = []
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
                boxes.append(box)

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