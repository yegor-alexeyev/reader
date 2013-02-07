#!/usr/bin/env sh
rm -f /dev/shm/video0-*
(camera/Release/camera > camera.log; date)&
LD_LIBRARY_PATH=/home/yegor/opencv-release-eclipse-project/lib preview/Release/preview&
autofocus/Release/autofocus&
trap 'killall autofocus; killall camera; killall preview' EXIT
cd /dev/shm
while true; do rm -f $(find -mmin +0.09 -and -name 'video0-*'); sleep 0.002; done
