cmake_minimum_required(VERSION 3.10)
project(leukocyte C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

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

# Include directories
include_directories(include)
include_directories(include/rules)
include_directories(include/allocator)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/vendor/prism/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/vendor/libyaml/include)

# Add Prism submodule (build with Makefile via ExternalProject)
include(ExternalProject)
ExternalProject_Add(prism_project
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/prism
    CONFIGURE_COMMAND ""
    # Export CPPFLAGS in the environment so the vendor Makefile uses them
    BUILD_COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR}/vendor/prism && env CPPFLAGS="-I${CMAKE_SOURCE_DIR}/include -I${CMAKE_SOURCE_DIR}/include/allocator" CFLAGS="${CFLAGS} -DPRISM_XALLOCATOR" make static
    BUILD_IN_SOURCE 1
    INSTALL_COMMAND ""
)

# Provide an imported target for the built static library so we can link against it
add_library(prism_static STATIC IMPORTED GLOBAL)
# Use static archive when available, otherwise fallback to shared object produced by the Prism build
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/vendor/prism/build/libprism.a)
    set(PRISM_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/vendor/prism/build/libprism.a)
else()
    set(PRISM_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/vendor/prism/build/libprism${CMAKE_SHARED_LIBRARY_SUFFIX})
endif()

set_target_properties(prism_static PROPERTIES
    IMPORTED_LOCATION ${PRISM_LIB_PATH}
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
    src/configs/raw_config.c
    src/configs/inherit.c
    src/configs/inherit_cache.c
    src/configs/merge.c
    src/configs/discover.c
    src/worker/pipeline.c
    src/category.c
    src/severity.c
    src/rules/rule_manager.c
    src/rules/rule.c
    src/rule_registry.c
    src/rules/layout/indentation_consistency.c
    src/io/file.c
    src/io/scan.c
    src/io/processed_source.c
    src/parse.c
    src/allocator/arena.c
    src/allocator/prism_xallocator.c
    src/diagnostic_debug_stub.c
    src/cli/formatter_bridge.c
    # Add more sources here
)

# IO tests
add_executable(test_io_scan tests/test_io_scan.c)
# Ensure leuko_prism_helpers follows prism_static so its symbols can satisfy
# unresolved references from libprism.a during static linking.
target_link_libraries(test_io_scan PRIVATE -Wl,--start-group prism_static leuko_lib leuko_prism_helpers -Wl,--end-group yaml)
add_test(NAME test_io_scan COMMAND test_io_scan)

# Generated config tests
add_executable(test_generated_config tests/test_generated_config.c)
# Link with --start-group/--end-group to resolve circular static references between Prism and leuko
target_link_libraries(test_generated_config PRIVATE -Wl,--start-group prism_static leuko_lib leuko_prism_helpers -Wl,--end-group yaml)
add_test(NAME test_generated_config COMMAND test_generated_config)

# YAML helpers tests
add_executable(test_yaml_helpers tests/test_yaml_helpers.c)
target_link_libraries(test_yaml_helpers PRIVATE -Wl,--start-group prism_static leuko_lib leuko_prism_helpers -Wl,--end-group yaml)
add_test(NAME test_yaml_helpers COMMAND test_yaml_helpers)

# Rule registry test
add_executable(test_rule_registry tests/test_rule_registry.c)
target_link_libraries(test_rule_registry PRIVATE -Wl,--start-group prism_static leuko_lib leuko_prism_helpers -Wl,--end-group yaml)
add_test(NAME test_rule_registry COMMAND test_rule_registry)

add_executable(test_rule_manager tests/test_rule_manager.c)
target_link_libraries(test_rule_manager PRIVATE -Wl,--start-group prism_static leuko_lib leuko_prism_helpers -Wl,--end-group yaml)
add_test(NAME test_rule_manager COMMAND test_rule_manager)

add_executable(test_rule_manager_threaded tests/test_rule_manager_threaded.c)
target_link_libraries(test_rule_manager_threaded PRIVATE -Wl,--start-group prism_static leuko_lib leuko_prism_helpers -Wl,--end-group yaml pthread)
add_test(NAME test_rule_manager_threaded COMMAND test_rule_manager_threaded)

# YAML case sensitivity tests
add_executable(test_yaml_case_sensitive tests/test_yaml_case_sensitive.c)
# Link Prism and leuko libraries as a group to allow mutual static references
target_link_libraries(test_yaml_case_sensitive PRIVATE -Wl,--start-group prism_static leuko_lib leuko_prism_helpers -Wl,--end-group yaml)
add_test(NAME test_yaml_case_sensitive COMMAND test_yaml_case_sensitive)

# Config loader tests
add_executable(test_config_loader tests/test_config_loader.c)
target_link_libraries(test_config_loader PRIVATE -Wl,--start-group prism_static leuko_lib leuko_prism_helpers -Wl,--end-group yaml)
add_test(NAME test_config_loader COMMAND test_config_loader)

# Loader fullname tests
add_executable(test_loader_fullname tests/test_loader_fullname.c)
target_link_libraries(test_loader_fullname PRIVATE -Wl,--start-group prism_static leuko_lib leuko_prism_helpers -Wl,--end-group yaml)
add_test(NAME test_loader_fullname COMMAND test_loader_fullname)

# Edge-case config loader tests
add_executable(test_config_loader_edgecases tests/test_config_loader_edgecases.c)
target_link_libraries(test_config_loader_edgecases PRIVATE -Wl,--start-group prism_static leuko_lib leuko_prism_helpers -Wl,--end-group yaml)
add_test(NAME test_config_loader_edgecases COMMAND test_config_loader_edgecases)

# Raw config file loader tests
add_executable(test_config_raw_loader tests/test_config_raw_loader.c)
target_link_libraries(test_config_raw_loader PRIVATE -Wl,--start-group prism_static leuko_lib leuko_prism_helpers -Wl,--end-group yaml)
add_test(NAME test_config_raw_loader COMMAND test_config_raw_loader)

# Inherit resolution tests
add_executable(test_config_inherit tests/test_config_inherit.c)
target_link_libraries(test_config_inherit PRIVATE -Wl,--start-group prism_static leuko_lib leuko_prism_helpers -Wl,--end-group yaml)
add_test(NAME test_config_inherit COMMAND test_config_inherit)

# Merge behavior tests
add_executable(test_config_merge tests/test_config_merge.c)
target_link_libraries(test_config_merge PRIVATE -Wl,--start-group prism_static leuko_lib leuko_prism_helpers -Wl,--end-group yaml)
add_test(NAME test_config_merge COMMAND test_config_merge)

# Map merge tests
add_executable(test_config_map_merge tests/test_config_map_merge.c)
target_link_libraries(test_config_map_merge PRIVATE -Wl,--start-group prism_static leuko_lib leuko_prism_helpers -Wl,--end-group yaml)
add_test(NAME test_config_map_merge COMMAND test_config_map_merge)

# Inherit cache tests
add_executable(test_config_inherit_cache tests/test_config_inherit_cache.c)
target_link_libraries(test_config_inherit_cache PRIVATE -Wl,--start-group prism_static leuko_lib leuko_prism_helpers -Wl,--end-group yaml)
add_test(NAME test_config_inherit_cache COMMAND test_config_inherit_cache)

# Discovery tests
add_executable(test_config_discover tests/test_config_discover.c)
target_link_libraries(test_config_discover PRIVATE -Wl,--start-group prism_static leuko_lib leuko_prism_helpers -Wl,--end-group yaml)
add_test(NAME test_config_discover COMMAND test_config_discover)

# Arena allocator tests
add_executable(test_arena tests/test_arena.c)
target_link_libraries(test_arena PRIVATE -Wl,--start-group prism_static leuko_lib leuko_prism_helpers -Wl,--end-group yaml)
add_test(NAME test_arena COMMAND test_arena)


add_executable(test_parser_arena tests/test_parser_arena.c)
target_link_libraries(test_parser_arena PRIVATE -Wl,--start-group prism_static leuko_lib leuko_prism_helpers -Wl,--end-group yaml)
add_test(NAME test_parser_arena COMMAND test_parser_arena)

add_executable(test_pipeline tests/test_pipeline.c)
target_link_libraries(test_pipeline PRIVATE -Wl,--start-group prism_static leuko_lib leuko_prism_helpers -Wl,--end-group yaml pthread)
add_test(NAME test_pipeline COMMAND test_pipeline)

# Library target for core sources so tests can link against it
add_library(leuko_lib STATIC ${SOURCES})

# Diagnostic free test
add_executable(test_diagnostic_free tests/test_diagnostic_free.c)
# Link order: ensure helper stubs and library are available before Prism static archive
target_link_libraries(test_diagnostic_free PRIVATE -Wl,--start-group prism_static leuko_lib leuko_prism_helpers -Wl,--end-group yaml)
add_test(NAME test_diagnostic_free COMMAND test_diagnostic_free)

# Small helper library containing symbols required to satisfy Prism's
# diagnostic references (keeps those objects available for link ordering)
add_library(leuko_prism_helpers STATIC src/diagnostic_debug_stub.c)

# Executable
add_executable(leuko src/main.c)
# Link order: ensure prism_static comes before leuko_lib so symbols provided by
# leukocyte (e.g. diagnostic stubs) satisfy references from Prism static lib.
target_link_libraries(leuko prism_static leuko_lib yaml pthread)

if (ENABLE_ASAN)
    target_compile_options(leuko PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(leuko PRIVATE -fsanitize=address)
endif()

if (ENABLE_GPROF)
    message(STATUS "Enabling gprof profiling (adding -pg)")
    target_compile_options(leuko PRIVATE -pg)
    target_link_options(leuko PRIVATE -pg)
    target_compile_options(leuko_lib PRIVATE -pg)
endif()

# Link libraries for library target
# Ensure helper stubs are linked before Prism static lib so their symbols satisfy
# references from the libprism.a archive during static linking.
target_link_libraries(leuko_lib leuko_prism_helpers prism_static yaml)

# Ensure the external Prism build is run before building leukocyte and library targets used by tests
add_dependencies(leuko prism_project)
add_dependencies(leuko_lib prism_project)
add_dependencies(test_parser_arena prism_project)

# Tests
enable_testing()
add_executable(test_leukocyte tests/test_main.c)
target_link_libraries(test_leukocyte PRIVATE -Wl,--start-group prism_static leuko_lib leuko_prism_helpers -Wl,--end-group yaml)
add_test(NAME test_leukocyte COMMAND test_leukocyte)

# CLI tests for comma-splitting and parsing
add_executable(test_cli tests/test_cli.c)
target_link_libraries(test_cli PRIVATE -Wl,--start-group prism_static leuko_lib leuko_prism_helpers -Wl,--end-group yaml)
add_test(NAME test_cli COMMAND test_cli)

# Processed source tests
add_executable(test_processed_source tests/test_processed_source.c)
# Use grouping to resolve references between prism and leuko libs
target_link_libraries(test_processed_source PRIVATE -Wl,--start-group prism_static leuko_lib leuko_prism_helpers -Wl,--end-group yaml)
add_test(NAME test_processed_source COMMAND test_processed_source)

# Position->line cache smoke test
add_executable(test_pos2line_cache tests/test_pos2line_cache.c)
target_link_libraries(test_pos2line_cache PRIVATE -Wl,--start-group prism_static leuko_lib leuko_prism_helpers -Wl,--end-group yaml)
add_test(NAME test_pos2line_cache COMMAND test_pos2line_cache)
