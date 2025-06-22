cmake_minimum_required(VERSION 3.10)
project(CameraMemtestApp)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(camera_memtest
    src/main.cpp
    src/OpenCVCamera.cpp
    src/FrameProcessor.cpp
    src/MemoryManager.cpp
)

find_package(OpenCV REQUIRED)
target_link_libraries(camera_memtest ${OpenCV_LIBS})