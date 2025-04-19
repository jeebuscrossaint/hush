cmake_minimum_required(VERSION 3.16)
project(hush C)

# Export compile_commands.json for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set C standard and optimization flags
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -flto -fno-plt -fno-exceptions -fomit-frame-pointer -pipe")

# Include the header directory
include_directories(include)

# Recursively get all .c files in the src directory and subdirectories
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.c)

# Define the executable
add_executable(hush ${SOURCES})

# Link with readline directly
target_link_libraries(hush readline)
