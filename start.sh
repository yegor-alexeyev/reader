#!/usr/bin/env sh
rm -f /dev/shm/video0-*
(camera/Release/camera > camera.log; date)&
autofocus/Release/autofocus&
./frames_cleaner.sh&
trap 'killall autofocus; killall camera; kill '$! EXIT
LD_LIBRARY_PATH=/home/yegor/opencv-release-eclipse-project/lib preview/Release/preview
