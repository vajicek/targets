#!/usr/bin/python3

import cv2
import numpy as np

cap = cv2.VideoCapture("http://192.168.1.129:8080/video")
#cap = cv2.VideoCapture("/dev/video0")

while(True):
	ret, frame = cap.read()
	cv2.imshow('frame',frame)
	if cv2.waitKey(1) & 0xFF == ord('q'):
		break

cap.release()
cv2.destroyAllWindows()