cmake_minimum_required(VERSION 3.10)
project(rpi5-cam-memtest CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenCV REQUIRED)

include_directories(
    ${OpenCV_INCLUDE_DIRS}
    /usr/include/libcamera
    include
)

add_executable(camera_memtest
    src/Main.cpp
    src/LibcameraCamera.cpp
)

find_library(LIBCAMERA_LIB camera)
find_library(LIBCAMERA_BASE_LIB camera-base)
target_link_libraries(camera_memtest ${OpenCV_LIBS} ${LIBCAMERA_LIB} ${LIBCAMERA_BASE_LIB})
