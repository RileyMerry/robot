#!/bin/bash

echo "Starting Camera + Visual Odometry..."

python3 /home/rmerry/robot_ws/projects/ugv_v2/scripts/camera_stream.py &

sleep 2

echo "Starting Robot Controller..."

cd /home/rmerry/robot_ws/projects/ugv_v2

./build/robot
