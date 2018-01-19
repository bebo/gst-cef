#!/bin/bash
set -e
make
rm -rf /home/neil/ignore/gstreamer-1.0/*
cp -Rfp build/dist/* ../ignore/venv/bin
cp -Rfp src/* /home/neil/ignore/gstreamer-1.0/
cp -Rfp src/.* /home/neil/ignore/gstreamer-1.0/
