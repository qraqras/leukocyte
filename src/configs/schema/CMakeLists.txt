# CMake for experimental schema-driven loader (opt-in)
if(NOT LEUKO_ENABLE_SCHEMA_EXPERIMENT)
    return()
endif()

add_library(leuko_schema STATIC layout_deser.c validator.c)
target_include_directories(leuko_schema PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Expose a convenience compile definition so code can conditionally use the schema loader
target_compile_definitions(leuko_schema PUBLIC LEUKO_ENABLE_SCHEMA_EXPERIMENT=1)

# Link into main library if present (best-effort)
if(TARGET leuko_lib)
    target_link_libraries(leuko_lib PRIVATE leuko_schema)
endif()

set_property(TARGET leuko_schema PROPERTY POSITION_INDEPENDENT_CODE ON)

# Install test helper when experiment is enabled
if(BUILD_TESTING)
    add_executable(test_schema_validate ../../tests/configs/test_schema_validation.c)
    target_link_libraries(test_schema_validate PRIVATE leuko_schema leuko_lib)
    add_test(NAME test_schema_validation COMMAND test_schema_validate)
endif()
