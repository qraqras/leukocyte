# leuko helpers for CMake
# Provides small helper macros / default compile settings used by this project.

# Set sane defaults for compiler options used across the project
macro(leuko_apply_common_compile_flags)
    # ASAN
    if(ENABLE_ASAN)
        message(STATUS "ENABLE_ASAN enabled: adding -fsanitize=address")
        add_compile_options(-fsanitize=address)
        add_link_options(-fsanitize=address)
    endif()

    # gprof
    if(ENABLE_GPROF)
        message(STATUS "ENABLE_GPROF enabled: adding -pg")
        add_compile_options(-pg)
        add_link_options(-pg)
    endif()
endmacro()

# Convenience: apply defaults immediately if called
leuko_apply_common_compile_flags()

# Helper to prefer vendor include paths if present
macro(leuko_add_vendor_includes)
    if(EXISTS "${CMAKE_SOURCE_DIR}/vendor/prism/include")
        include_directories(${CMAKE_SOURCE_DIR}/vendor/prism/include)
    endif()
    if(EXISTS "${CMAKE_SOURCE_DIR}/vendor/libyaml/include")
        include_directories(${CMAKE_SOURCE_DIR}/vendor/libyaml/include)
    endif()
endmacro()

leuko_add_vendor_includes()
