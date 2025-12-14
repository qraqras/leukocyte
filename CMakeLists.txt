cmake_minimum_required(VERSION 3.10)
project(leukocyte C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Option to enable address sanitizer for debugging
option(ENABLE_ASAN "Enable Address Sanitizer for leukocyte target" OFF)

# Include directories
include_directories(include)
include_directories(include/rules)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/vendor/prism/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/vendor/libyaml/include)

# Add Prism submodule (build with Makefile via ExternalProject)
include(ExternalProject)
ExternalProject_Add(prism_project
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/prism
    CONFIGURE_COMMAND ""
    BUILD_COMMAND make static
    BUILD_IN_SOURCE 1
    INSTALL_COMMAND ""
)

# Provide an imported target for the built static library so we can link against it
add_library(prism_static STATIC IMPORTED GLOBAL)
set_target_properties(prism_static PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/vendor/prism/build/libprism.a
    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/vendor/prism/include
)

# Add libyaml submodule
add_subdirectory(vendor/libyaml)

# Source files
set(SOURCES
    src/main.c
    src/cli/cli.c
    src/configs/config.c
    src/configs/config_file.c
    src/configs/config_defaults.c
    src/configs/yaml_reader.c
    src/configs/layout/indentation_consistency.c
    src/rules/rule_manager.c
    src/rules/layout/indentation_consistency.c
    # Add more sources here
)

# Library target for core sources so tests can link against it
add_library(leuko_lib STATIC ${SOURCES})

# Executable
add_executable(leuko src/main.c)
target_link_libraries(leuko leuko_lib)

if (ENABLE_ASAN)
    target_compile_options(leuko PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(leuko PRIVATE -fsanitize=address)
endif()

# Link libraries
# Use the imported prism_static target which depends on the ExternalProject build
target_link_libraries(leuko prism_static yaml)
target_link_libraries(leuko_lib prism_static yaml)

# Ensure the external Prism build is run before building leukocyte
add_dependencies(leuko prism_project)

# Tests
enable_testing()
add_executable(test_leukocyte tests/test_main.c)
target_link_libraries(test_leukocyte leuko_lib yaml)
add_test(NAME test_leukocyte COMMAND test_leukocyte)
