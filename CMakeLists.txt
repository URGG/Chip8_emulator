cmake_minimum_required(VERSION 3.20)
project(Chip8_Emulator C)

set(CMAKE_C_STANDARD 11)

# Tell CMake to look in the src directory
include_directories(src)
find_package(SDL2 REQUIRED)

include_directories(${SDL2_INCLUDE_DIRS})
# Add your source files
add_executable(Chip8_Emulator
        src/main.c
        src/chip8.c
        src/chip8.c
        src/platform.h

)

target_link_libraries(Chip8_Emulator ${SDL2_LIBRARIES})