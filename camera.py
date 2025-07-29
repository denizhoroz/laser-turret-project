import cv2 as cv
import numpy as np

cap = cv.VideoCapture(0)

LOW_BLACK = np.array([0, 120, 120], np.uint8)
HIGH_BLACK = np.array([10, 255, 255], np.uint8)

while True:
    ret, frame = cap.read()
    if ret:
        frame_hsv = cv.cvtColor(frame, cv.COLOR_BGR2HSV)
        
        mask = cv.inRange(frame_hsv, LOW_BLACK, HIGH_BLACK)
        mask = cv.GaussianBlur(src=mask, ksize=(3, 3), sigmaX=2)
        mask = cv.erode(src=mask, kernel=(5, 5), iterations=2)
        mask = cv.dilate(src=mask, kernel=(5, 5), iterations=2)

        (contours, _) = cv.findContours(mask.copy(), cv.RETR_TREE, cv.CHAIN_APPROX_SIMPLE)

        cv.drawContours(frame, contours, -1, (0, 255, 0), 2)
        cv.imshow('frame', frame)




    if cv.waitKey(25) & 0xFF == ord('q'):
        break

    else:
        print("Returned None from capture object")

cap.release()
cv.destroyAllWindows()