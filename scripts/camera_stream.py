from flask import Flask, Response
from picamera2 import Picamera2
import cv2
import time

app = Flask(__name__)

picam2 = Picamera2()
config = picam2.create_video_configuration(
    main={"size": (640, 480), "format": "RGB888"}
)
picam2.configure(config)
picam2.start()
time.sleep(1)

def generate_frames():
    while True:
        frame = picam2.capture_array()
        success, buffer = cv2.imencode(".jpg", frame)

        if not success:
            continue

        jpg = buffer.tobytes()

        yield (
            b"--frame\r\n"
            b"Content-Type: image/jpeg\r\n\r\n" + jpg + b"\r\n"
        )

@app.route("/")
def index():
    return "Camera stream running. Go to /stream"

@app.route("/stream")
def stream():
    return Response(
        generate_frames(),
        mimetype="multipart/x-mixed-replace; boundary=frame"
    )

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8081)
