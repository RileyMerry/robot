import cv2
import numpy as np
from picamera2 import Picamera2
import time

# -----------------------
# Camera Setup
# -----------------------
picam2 = Picamera2()

config = picam2.create_preview_configuration(
    main={"size": (640, 480), "format": "RGB888"}
)

picam2.configure(config)
picam2.start()

time.sleep(2)

# -----------------------
# Camera Matrix
# -----------------------
fx = 500
fy = 500
cx = 320
cy = 240

K = np.array([
    [fx, 0, cx],
    [0, fy, cy],
    [0, 0, 1]
], dtype=np.float64)

# -----------------------
# Feature Detector
# -----------------------
orb = cv2.ORB_create(1500)

bf = cv2.BFMatcher(
    cv2.NORM_HAMMING,
    crossCheck=True
)

# -----------------------
# Pose Variables
# -----------------------
x = 0.0
z = 0.0

prev_kp = None
prev_des = None
prev_gray = None

frame_count = 0

print("Visual SLAM Started")

while True:

    frame = picam2.capture_array()

    gray = cv2.cvtColor(
        frame,
        cv2.COLOR_RGB2GRAY
    )

    kp, des = orb.detectAndCompute(
        gray,
        None
    )

    if (
        prev_gray is not None
        and prev_des is not None
        and des is not None
    ):

        matches = bf.match(
            prev_des,
            des
        )

        matches = sorted(
            matches,
            key=lambda m: m.distance
        )

        good_matches = matches[:100]

        if len(good_matches) > 20:

            pts_prev = np.float32([
                prev_kp[m.queryIdx].pt
                for m in good_matches
            ])

            pts_curr = np.float32([
                kp[m.trainIdx].pt
                for m in good_matches
            ])

            E, mask = cv2.findEssentialMat(
                pts_curr,
                pts_prev,
                K,
                method=cv2.RANSAC,
                prob=0.999,
                threshold=1.0
            )

            if E is not None:

                _, R, t, mask = cv2.recoverPose(
                    E,
                    pts_curr,
                    pts_prev,
                    K
                )

                scale = 0.1

                dx = float(t[0]) * scale
                dz = float(t[2]) * scale

                x += dx
                z += dz

                frame_count += 1

                if frame_count % 10 == 0:

                    print(
                        f"Features: {len(kp)} | "
                        f"Matches: {len(good_matches)} | "
                        f"X: {x:.2f} | "
                        f"Z: {z:.2f}"
                    )

    prev_gray = gray
    prev_kp = kp
    prev_des = des

    time.sleep(0.03)
