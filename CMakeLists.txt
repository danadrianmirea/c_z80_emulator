# Root CMakeLists.txt
cmake_minimum_required(VERSION 3.15)  # Minimum CMake version
project(zx_emulator LANGUAGES C)      # Project name and language

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Add source subdirectory
add_subdirectory(src)