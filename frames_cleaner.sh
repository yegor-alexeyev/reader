#!/usr/bin/env sh
cd /tmp
while true; do rm -f $(find -mmin +0.09 -and -name 'video0-*'); sleep 0.002; done
