cmake_minimum_required(VERSION 3.20)
project(brepgpu_step_source LANGUAGES CXX)
find_package(OpenCASCADE REQUIRED)
add_executable(brepgpu-step-source main.cpp)
target_compile_features(brepgpu-step-source PRIVATE cxx_std_17)
target_include_directories(brepgpu-step-source PRIVATE ${OpenCASCADE_INCLUDE_DIR})
target_link_directories(brepgpu-step-source PRIVATE ${OpenCASCADE_LIBRARY_DIR})
target_link_libraries(brepgpu-step-source PRIVATE TKDESTEP TKXCAF TKCAF TKLCAF
  TKCDF TKXSBase TKShHealing TKTopAlgo TKGeomAlgo TKBRep TKGeomBase TKG3d
  TKG2d TKMath TKernel)
target_compile_options(brepgpu-step-source PRIVATE -Wall -Wextra -Wpedantic)
if(UNIX AND NOT APPLE)
  # Keep the isolated prefix available to transitive OCCT toolkit dependencies.
  target_link_options(brepgpu-step-source PRIVATE -Wl,--disable-new-dtags)
endif()
set_target_properties(brepgpu-step-source PROPERTIES
  BUILD_RPATH "${OpenCASCADE_LIBRARY_DIR}")
