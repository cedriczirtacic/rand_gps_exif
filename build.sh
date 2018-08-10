#!/bin/bash

cmake . && make
gcc -Wall -I. -I./libjpeg -o rand_gps_exif libjpeg.a -lexif rand_gps_exif.c 

