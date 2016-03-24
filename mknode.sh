#!/bin/sh

MAJOR=$(cat /proc/devices | grep globalmem | awk '{print $1}')
mknod /dev/gblm c $MAJOR 0
