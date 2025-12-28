cmake_minimum_required(VERSION 3.10)
project(leukocyte C)

option(LEUKO_ENABLE_SCHEMA_EXPERIMENT "Enable experimental schema-driven JSON loader" OFF)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Use local CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(LeukoHelpers OPTIONAL)

# Default to Release build type (for single-config generators) when not provided
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
    message(STATUS "CMAKE_BUILD_TYPE not set; defaulting to Release")
endif()

# Ensure Release builds use -O3 and define NDEBUG
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -DNDEBUG")

# Option to enable address sanitizer for debugging
option(ENABLE_ASAN "Enable Address Sanitizer for leukocyte target" OFF)
# Option to enable gprof profiling (adds -pg to compile and link flags)
option(ENABLE_GPROF "Enable gprof profiling for leukocyte target" OFF)

# Optionally build experimental schema-driven loader
if(LEUKO_ENABLE_SCHEMA_EXPERIMENT)
    add_subdirectory(src/configs/schema)
endif()

# Defer the rest of the build into the `src/` subdirectory for clarity
add_subdirectory(src)

# Tests directory (top-level)
enable_testing()
if(EXISTS ${CMAKE_SOURCE_DIR}/tests/CMakeLists.txt)
    add_subdirectory(tests)
endif()
