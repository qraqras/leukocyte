# CMake for experimental schema-driven loader (opt-in)
if(NOT LEUKO_ENABLE_SCHEMA_EXPERIMENT)
    return()
endif()

add_library(leuko_schema STATIC layout_deser.c)
target_include_directories(leuko_schema PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Expose a convenience compile definition so code can conditionally use the schema loader
target_compile_definitions(leuko_schema PUBLIC LEUKO_ENABLE_SCHEMA_EXPERIMENT=1)

# Link into main library if present (best-effort)
if(TARGET leuko_lib)
    target_link_libraries(leuko_lib PRIVATE leuko_schema)
endif()

set_property(TARGET leuko_schema PROPERTY POSITION_INDEPENDENT_CODE ON)
