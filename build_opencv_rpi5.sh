#!/bin/bash
set -e

echo "[1/6] Updating system..."
sudo apt update && sudo apt upgrade -y

echo "[2/6] Installing dependencies..."
sudo apt install -y \
  build-essential cmake git pkg-config \
  libjpeg-dev libpng-dev libtiff-dev \
  libavcodec-dev libavformat-dev libswscale-dev \
  libv4l-dev v4l-utils \
  libxvidcore-dev libx264-dev \
  libgtk-3-dev libatlas-base-dev gfortran \
  python3-dev python3-numpy \
  libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
  libcamera-dev libcamera-apps

echo "[3/6] Cloning OpenCV sources..."
cd ~
git clone https://github.com/opencv/opencv.git
git clone https://github.com/opencv/opencv_contrib.git
cd opencv && git checkout 4.8.0
cd ../opencv_contrib && git checkout 4.8.0

echo "[4/6] Configuring with CMake..."
cd ~/opencv
mkdir -p build && cd build

cmake -D CMAKE_BUILD_TYPE=RELEASE \
      -D CMAKE_INSTALL_PREFIX=/usr/local \
      -D OPENCV_EXTRA_MODULES_PATH=~/opencv_contrib/modules \
      -D ENABLE_NEON=ON \
      -D WITH_V4L=ON \
      -D WITH_GSTREAMER=ON \
      -D WITH_LIBCAMERA=ON \
      -D WITH_OPENGL=ON \
      -D BUILD_EXAMPLES=OFF ..

echo "[5/6] Building OpenCV..."
make -j$(nproc)

echo "[6/6] Installing OpenCV..."
sudo make install
sudo ldconfig

echo "DONE - OpenCV is installed with V4L2 and GStreamer support."
