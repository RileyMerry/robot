from flask import Flask, Response, jsonify
from picamera2 import Picamera2
import cv2
import numpy as np
import time
import threading

app = Flask(__name__)

# -----------------------
# Camera setup
# -----------------------
picam2 = Picamera2()
config = picam2.create_video_configuration(
    main={"size": (640, 480), "format": "RGB888"}
)
picam2.configure(config)
picam2.start()
time.sleep(1)

# -----------------------
# SLAM setup
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

orb = cv2.ORB_create(1500)
bf = cv2.BFMatcher(cv2.NORM_HAMMING, crossCheck=True)

prev_kp = None
prev_des = None

x = 300.0
z = 300.0

slam_x = 0.0
slam_z = 0.0
feature_count = 0
match_count = 0

trajectory = np.zeros((600, 600, 3), dtype=np.uint8)

latest_frame = None
latest_map = None

lock = threading.Lock()


def process_slam(frame):
    global prev_kp, prev_des
    global x, z, slam_x, slam_z
    global feature_count, match_count
    global trajectory, latest_map

    gray = cv2.cvtColor(frame, cv2.COLOR_RGB2GRAY)

    kp, des = orb.detectAndCompute(gray, None)
    feature_count = len(kp) if kp is not None else 0

    if prev_des is not None and des is not None and prev_kp is not None:
        matches = bf.match(prev_des, des)
        matches = sorted(matches, key=lambda m: m.distance)
        good_matches = matches[:100]
        match_count = len(good_matches)

        if len(good_matches) > 20:
            pts_prev = np.float32([
                prev_kp[m.queryIdx].pt for m in good_matches
            ])

            pts_curr = np.float32([
                kp[m.trainIdx].pt for m in good_matches
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
                _, R, t, mask_pose = cv2.recoverPose(
                    E,
                    pts_curr,
                    pts_prev,
                    K
                )

                scale = 5.0

                dx = float(t[0, 0]) * scale
                dz = float(t[2, 0]) * scale

                x += dx
                z += dz

                slam_x += dx
                slam_z += dz

                px = int(max(0, min(599, x)))
                pz = int(max(0, min(599, z)))

                cv2.circle(trajectory, (px, pz), 2, (255, 255, 255), -1)

    prev_kp = kp
    prev_des = des

    map_copy = trajectory.copy()

    cv2.circle(map_copy, (int(x), int(z)), 6, (255, 255, 255), -1)
    cv2.putText(
        map_copy,
        f"X: {slam_x:.2f}  Z: {slam_z:.2f}",
        (20, 30),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.7,
        (255, 255, 255),
        2
    )
    cv2.putText(
        map_copy,
        f"Features: {feature_count}  Matches: {match_count}",
        (20, 60),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.6,
        (255, 255, 255),
        2
    )

    latest_map = map_copy


def generate_frames():
    global latest_frame

    while True:
        frame = picam2.capture_array()

        process_slam(frame)

        with lock:
            latest_frame = frame.copy()

        success, buffer = cv2.imencode(".jpg", frame)

        if not success:
            continue

        jpg = buffer.tobytes()

        yield (
            b"--frame\r\n"
            b"Content-Type: image/jpeg\r\n\r\n" + jpg + b"\r\n"
        )


def generate_slam_map():
    while True:
        if latest_map is None:
            blank = np.zeros((600, 600, 3), dtype=np.uint8)
            cv2.putText(
                blank,
                "SLAM initializing...",
                (150, 300),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.8,
                (255, 255, 255),
                2
            )
            frame = blank
        else:
            frame = latest_map

        success, buffer = cv2.imencode(".jpg", frame)

        if not success:
            continue

        jpg = buffer.tobytes()

        yield (
            b"--frame\r\n"
            b"Content-Type: image/jpeg\r\n\r\n" + jpg + b"\r\n"
        )

        time.sleep(0.1)


@app.route("/")
def index():
    return "Camera stream running. Go to /stream or /slam_map"


@app.route("/stream")
def stream():
    return Response(
        generate_frames(),
        mimetype="multipart/x-mixed-replace; boundary=frame"
    )


@app.route("/slam_map")
def slam_map():
    return Response(
        generate_slam_map(),
        mimetype="multipart/x-mixed-replace; boundary=frame"
    )


@app.route("/slam_data")
def slam_data():
    return jsonify({
        "x": round(slam_x, 2),
        "z": round(slam_z, 2),
        "features": feature_count,
        "matches": match_count
    })


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8081, threaded=True)
