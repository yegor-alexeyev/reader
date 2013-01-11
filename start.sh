#!/usr/bin/env sh
rm /dev/mqueue/video0-*
camera/Release/camera&
LD_LIBRARY_PATH=/home/yegor/opencv-release-eclipse-project/lib autofocus/Release/autofocus
killall camera


