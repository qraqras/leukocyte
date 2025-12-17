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
    src/cli/formatter.c
    src/configs/config.c
    src/configs/layout/indentation_consistency.c
    src/configs/generated_config.c
    src/configs/yaml_helpers.c
    src/configs/loader.c
    src/category.c
    src/severity.c
    src/rules/rule_manager.c
    src/rules/rule.c
    src/rule_registry.c
    src/rules/layout/indentation_consistency.c
    src/io/file.c
    src/io/scan.c
    src/parse.c
    # Add more sources here
)

# IO tests
add_executable(test_io_scan tests/test_io_scan.c)
target_link_libraries(test_io_scan leuko_lib yaml)
add_test(NAME test_io_scan COMMAND test_io_scan)

# Generated config tests
add_executable(test_generated_config tests/test_generated_config.c)
target_link_libraries(test_generated_config leuko_lib yaml)
add_test(NAME test_generated_config COMMAND test_generated_config)

# YAML helpers tests
add_executable(test_yaml_helpers tests/test_yaml_helpers.c)
target_link_libraries(test_yaml_helpers leuko_lib yaml)
add_test(NAME test_yaml_helpers COMMAND test_yaml_helpers)

# Rule registry test
add_executable(test_rule_registry tests/test_rule_registry.c)
target_link_libraries(test_rule_registry leuko_lib yaml)
add_test(NAME test_rule_registry COMMAND test_rule_registry)

# YAML case sensitivity tests
add_executable(test_yaml_case_sensitive tests/test_yaml_case_sensitive.c)
target_link_libraries(test_yaml_case_sensitive leuko_lib yaml)
add_test(NAME test_yaml_case_sensitive COMMAND test_yaml_case_sensitive)

# Config loader tests
add_executable(test_config_loader tests/test_config_loader.c)
target_link_libraries(test_config_loader leuko_lib yaml)
add_test(NAME test_config_loader COMMAND test_config_loader)

# Loader fullname tests
add_executable(test_loader_fullname tests/test_loader_fullname.c)
target_link_libraries(test_loader_fullname leuko_lib yaml)
add_test(NAME test_loader_fullname COMMAND test_loader_fullname)

# Edge-case config loader tests
add_executable(test_config_loader_edgecases tests/test_config_loader_edgecases.c)
target_link_libraries(test_config_loader_edgecases leuko_lib yaml)
add_test(NAME test_config_loader_edgecases COMMAND test_config_loader_edgecases)

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

# CLI tests for comma-splitting and parsing
add_executable(test_cli tests/test_cli.c)
target_link_libraries(test_cli leuko_lib yaml)
add_test(NAME test_cli COMMAND test_cli)
