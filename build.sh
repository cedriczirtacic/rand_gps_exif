#!/bin/bash
setopt -e
echo "Building libjpeg..."
cmake . && make

echo "Building rand_gps_exif..."
gcc -Wall -I. -I./libjpeg -o rand_gps_exif libjpeg.a -lexif rand_gps_exif.c \
    && file rand_gps_exif

