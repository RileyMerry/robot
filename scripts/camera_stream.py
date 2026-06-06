from flask import Flask, Response, jsonify
from picamera2 import Picamera2
import cv2
import numpy as np
import time
import threading

app = Flask(__name__)

# -----------------------
# Fast Performance Settings
# -----------------------
WIDTH = 320
HEIGHT = 240
JPEG_QUALITY = 40
SLAM_EVERY_N_FRAMES = 6
MAP_WIDTH = 400
MAP_HEIGHT = 400

# -----------------------
# Camera Setup
# -----------------------
picam2 = Picamera2()
config = picam2.create_video_configuration(
    main={"size": (WIDTH, HEIGHT), "format": "RGB888"},
    buffer_count=2
)
picam2.configure(config)
picam2.start()
time.sleep(1)

# -----------------------
# Camera Matrix
# -----------------------
fx = 260
fy = 260
cx = WIDTH / 2
cy = HEIGHT / 2

K = np.array([
    [fx, 0, cx],
    [0, fy, cy],
    [0, 0, 1]
], dtype=np.float64)

# -----------------------
# Optimized ORB Setup
# -----------------------
orb = cv2.ORB_create(
    nfeatures=400,
    scaleFactor=1.2,
    nlevels=4,
    fastThreshold=25
)

bf = cv2.BFMatcher(cv2.NORM_HAMMING, crossCheck=True)

prev_kp = None
prev_des = None

draw_x = MAP_WIDTH / 2
draw_z = MAP_HEIGHT / 2

slam_x = 0.0
slam_z = 0.0
feature_count = 0
match_count = 0
frame_counter = 0

trajectory = np.zeros((MAP_HEIGHT, MAP_WIDTH, 3), dtype=np.uint8)
latest_map = trajectory.copy()

lock = threading.Lock()


def process_slam(frame):
    global prev_kp, prev_des
    global draw_x, draw_z
    global slam_x, slam_z
    global feature_count, match_count
    global latest_map

    gray = cv2.cvtColor(frame, cv2.COLOR_RGB2GRAY)

    kp, des = orb.detectAndCompute(gray, None)
    feature_count = len(kp) if kp is not None else 0

    if prev_des is not None and des is not None and prev_kp is not None:
        matches = bf.match(prev_des, des)
        matches = sorted(matches, key=lambda m: m.distance)
        good_matches = matches[:35]
        match_count = len(good_matches)

        if len(good_matches) > 18:
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

                scale = 3.5

                dx = float(t[0, 0]) * scale
                dz = float(t[2, 0]) * scale

                draw_x += dx
                draw_z += dz

                slam_x += dx
                slam_z += dz

                px = int(max(0, min(MAP_WIDTH - 1, draw_x)))
                pz = int(max(0, min(MAP_HEIGHT - 1, draw_z)))

                cv2.circle(trajectory, (px, pz), 2, (255, 255, 255), -1)

    prev_kp = kp
    prev_des = des

    map_copy = trajectory.copy()

    robot_px = int(max(0, min(MAP_WIDTH - 1, draw_x)))
    robot_pz = int(max(0, min(MAP_HEIGHT - 1, draw_z)))

    cv2.circle(map_copy, (robot_px, robot_pz), 5, (255, 255, 255), -1)

    cv2.putText(
        map_copy,
        f"X: {slam_x:.2f}  Z: {slam_z:.2f}",
        (12, 24),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.5,
        (255, 255, 255),
        1
    )

    cv2.putText(
        map_copy,
        f"Features: {feature_count}  Matches: {match_count}",
        (12, 46),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.45,
        (255, 255, 255),
        1
    )

    with lock:
        latest_map = map_copy


def generate_frames():
    global frame_counter

    encode_params = [int(cv2.IMWRITE_JPEG_QUALITY), JPEG_QUALITY]

    while True:
        frame = picam2.capture_array()

        frame_counter += 1

        if frame_counter % SLAM_EVERY_N_FRAMES == 0:
            process_slam(frame)

        success, buffer = cv2.imencode(
            ".jpg",
            frame,
            encode_params
        )

        if not success:
            continue

        jpg = buffer.tobytes()

        yield (
            b"--frame\r\n"
            b"Content-Type: image/jpeg\r\n\r\n" + jpg + b"\r\n"
        )

        time.sleep(0.015)


def generate_slam_map():
    encode_params = [int(cv2.IMWRITE_JPEG_QUALITY), JPEG_QUALITY]

    while True:
        with lock:
            frame = latest_map.copy()

        success, buffer = cv2.imencode(
            ".jpg",
            frame,
            encode_params
        )

        if not success:
            continue

        jpg = buffer.tobytes()

        yield (
            b"--frame\r\n"
            b"Content-Type: image/jpeg\r\n\r\n" + jpg + b"\r\n"
        )

        time.sleep(0.25)


@app.route("/")
def index():
    return "Camera and SLAM server running. Use /stream, /slam_map, or /slam_data."


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
