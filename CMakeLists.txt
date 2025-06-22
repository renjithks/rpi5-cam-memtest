cmake_minimum_required(VERSION 3.10)
project(rpi5-cam-memtest CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenCV REQUIRED)

include_directories(
    ${OpenCV_INCLUDE_DIRS}
    /usr/include/libcamera
)

add_executable(camera_memtest
    src/main.cpp
    src/LibcameraCamera.cpp
)

target_link_libraries(camera_memtest
    ${OpenCV_LIBS}
    libcamera
)
