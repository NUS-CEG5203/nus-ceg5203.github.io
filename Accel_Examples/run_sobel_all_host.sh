#!/bin/bash
# Builds and runs the naive / tiled / image-object Sobel OpenCL comparison.
# Run from this directory (the .cl kernels are loaded at runtime):
#   bash run_sobel_all_host.sh
# Image size (1024x1024) and work-group size (16x16) are fixed in sobel_all_host.c.
set -e
cd "$(dirname "$0")"
gcc sobel_all_host.c -o sobel_all_host -lOpenCL -O2 -lm
./sobel_all_host
