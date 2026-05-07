from packages.config import TCAREA_RATIO


class Rectangle:
    """A rectangle class."""
    
    def __init__(self, x: int, y: int, width: int, height: int):
        self.x = x
        self.y = y
        self.width = width
        self.height = height
        self.TCPoint = None
        self.TCArea = None

class Tracker:
    """Tracker for measuring the position of the target in the frame and calculating the necessary angles for the turret to aim at the target."""

    def __init__(self, window_size: tuple = (640, 480), tcarea_ratio: float = TCAREA_RATIO):
        """tcarea_ratio: side length of TCArea as fraction of bbox side."""
        self.window_size = window_size
        self.center = (self.window_size[0] // 2, self.window_size[1] // 2)
        self.tcarea_ratio = tcarea_ratio

    def track(self, box):
        """Tracks the target based on the detected bounding box and calculates the necessary angles for the turret.

        Args:
            box (dict): Detected bounding box and metadata.
        """

        # Define the center of the ROI and the center area of the detected box
        x1, y1, x2, y2 = map(int, box.xyxy)

        # - Target Center Point
        self.TCPoint = ((x1 + x2) // 2, (y1 + y2) // 2)

        # - Target Center Area: rectangle scaled to bbox size via tcarea_ratio.
        bbox_w = max(1, x2 - x1)
        bbox_h = max(1, y2 - y1)
        tc_w = max(1, int(bbox_w * self.tcarea_ratio))
        tc_h = max(1, int(bbox_h * self.tcarea_ratio))
        self.TCArea = Rectangle(
            x=self.TCPoint[0] - tc_w // 2,
            y=self.TCPoint[1] - tc_h // 2,
            width=tc_w,
            height=tc_h,
        )

        # Calculate offset from the center of the screen to the center area of the detected box
        offset_x = self.TCPoint[0] - self.center[0]
        offset_y = self.TCPoint[1] - self.center[1]
        offset = (offset_x, offset_y)

        # Check if the crosshair area is within the target center area
        in_roi = (self.center[0] >= self.TCArea.x and self.center[0] <= self.TCArea.x + self.TCArea.width and
                  self.center[1] >= self.TCArea.y and self.center[1] <= self.TCArea.y + self.TCArea.height)

        return offset, in_roi
