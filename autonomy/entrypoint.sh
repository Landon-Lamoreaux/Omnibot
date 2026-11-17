#!/bin/bash -i

# The -i flag allows bashrc to be sourced in the container.

echo entrypoint.sh

# Source ROS and packages
. /root/.bashrc

# Activate python venv
. /venv/bin/activate

# Setting up display forwarding for viewing rviz over ssh.
if [ "$VNC_DISPLAY" -eq 1 ]; then
  Xvfb :1 -screen 0 1600x1200x24 &
  export DISPLAY=:1.0

  sleep 5

  x11vnc -display :1 -forever -nopw -listen 0.0.0.0 -rfbport 5900 &

else
    echo "VNC_DISPLAY != 1"
    echo "Using host X11 display"

    export DISPLAY="${DISPLAY:-:0}"

    echo "DISPLAY = ${DISPLAY}"
fi

# Launch ROS nodes.
ros2 launch launch_files autonomy.launch.py
