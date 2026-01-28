cmake_minimum_required(VERSION 3.20)
project(Chip8_Emulator C)

set(CMAKE_C_STANDARD 11)

# Tell CMake where Homebrew lives on Apple Silicon
list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")

find_package(SDL2 REQUIRED)

add_executable(Chip8_Emulator
        src/main.c
        src/chip8.c
        src/platform.c
        src/chip8.h
        src/platform.h
)

# Link using the modern SDL2 targets
target_link_libraries(Chip8_Emulator SDL2::SDL2)