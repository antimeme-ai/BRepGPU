cmake_minimum_required(VERSION 3.20)
project(brepgpu_measurement LANGUAGES C)
find_package(Threads REQUIRED)
# Delivered-clock probe used by scripts/measurement_lane.py context rows.
add_executable(brepgpu-delivered-clock delivered_clock.c)
set_target_properties(brepgpu-delivered-clock PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
target_compile_options(brepgpu-delivered-clock PRIVATE -O2 -Wall -Wextra -Werror)
target_link_libraries(brepgpu-delivered-clock PRIVATE Threads::Threads)
