cmake_minimum_required(VERSION 3.16)
project(CameraDmaTest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Debug vs Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Libcamera configuration
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBCAMERA REQUIRED IMPORTED_TARGET libcamera)
pkg_check_modules(LIBCAMERA_BASE REQUIRED IMPORTED_TARGET libcamera-base)

# Linux dependencies
find_package(Threads REQUIRED)

# Source files
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
    "src/dma/*.cpp"
    "src/camera/*.cpp"
    "src/monitoring/*.cpp"
)

# Application executable
add_executable(camera_dma_test ${SOURCES})

# Include directories
target_include_directories(camera_dma_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# Link libraries
target_link_libraries(camera_dma_test
    PRIVATE
        PkgConfig::LIBCAMERA
        PkgConfig::LIBCAMERA_BASE
        Threads::Threads
)

# Compiler flags
target_compile_options(camera_dma_test PRIVATE
    -Wall -Wextra -Wpedantic
    -O3 -march=armv8-a+crc+simd -mtune=cortex-a76
)

# Installation
install(TARGETS camera_dma_test DESTINATION bin)