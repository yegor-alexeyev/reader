#!/usr/bin/env sh
cd /dev/shm
while true; do rm -f $(find -mmin +0.09 -and -name 'video0-*'); sleep 0.002; done
