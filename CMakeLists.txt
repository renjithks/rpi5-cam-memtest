cmake_minimum_required(VERSION 3.10)
project(CameraMemTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)

include_directories(include)

add_executable(camera_memtest
    src/main.cpp
    src/OpenCVCamera.cpp
    src/FrameProcessor.cpp
    src/MemoryManager.cpp
)

target_link_libraries(camera_memtest PRIVATE ${OpenCV_LIBS})
