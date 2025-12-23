# Project: src/CMakeLists.txt (simplified, auto-collection)
# Collect C sources under src/ recursively and build library and executable

# Auto-discover include directories under include/ (recursively)
include_directories(${CMAKE_SOURCE_DIR}/include)
file(GLOB_RECURSE LEUKO_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/include/*")
foreach(_inc ${LEUKO_INCLUDE_DIRS})
    if(IS_DIRECTORY "${_inc}")
        include_directories("${_inc}")
    endif()
endforeach()

# Include optional helpers
include(Prism OPTIONAL)
include(LibYAML OPTIONAL)
include(LibUV OPTIONAL)

# Automatically collect .c sources under src/
file(GLOB_RECURSE LEUKO_SOURCES_REL "${CMAKE_SOURCE_DIR}/src/*.c")
# Exclude main.c from library sources (it is used for the executable)
list(REMOVE_ITEM LEUKO_SOURCES_REL "${CMAKE_SOURCE_DIR}/src/main.c")

# If no sources found, fall back to a small stub to keep builds working
if(LEUKO_SOURCES_REL)
    message(STATUS "Collected leuko sources: ${LEUKO_SOURCES_REL}")
    add_library(leuko_lib STATIC ${LEUKO_SOURCES_REL})
else()
    message(STATUS "No sources found under src/; using src/leuko_stubs.c placeholder")
    add_library(leuko_lib STATIC ${CMAKE_SOURCE_DIR}/src/leuko_stubs.c)
endif()

# Provide small helper library for Prism symbol satisfaction
# Previously we conditionally created a STATIC helper from diagnostic_debug_stub.c
# or leuko_stubs.c. With auto-collection, allow a no-op INTERFACE target for
# compatibility so consumers can still link against `leuko_prism_helpers`.
add_library(leuko_prism_helpers INTERFACE)

# Main executable (if present)
if(EXISTS ${CMAKE_SOURCE_DIR}/src/main.c)
    add_executable(leuko ${CMAKE_SOURCE_DIR}/src/main.c)
    # Link order chosen to match previous setup
    if(TARGET prism_static)
        if(TARGET yaml)
            target_link_libraries(leuko PRIVATE prism_static leuko_lib yaml pthread)
        else()
            target_link_libraries(leuko PRIVATE prism_static leuko_lib pthread)
        endif()
    else()
        if(TARGET yaml)
            target_link_libraries(leuko PRIVATE leuko_lib yaml pthread)
        else()
            target_link_libraries(leuko PRIVATE leuko_lib pthread)
        endif()
    endif()
endif()

# If libuv is being built via ExternalProject, ensure library target depends on it
if(TARGET libuv_ep)
    add_dependencies(leuko_lib libuv_ep)
endif()

# Link libraries for library target (conditionally add prism/yaml)
if(TARGET prism_static)
    if(TARGET yaml)
        target_link_libraries(leuko_lib PRIVATE leuko_prism_helpers prism_static yaml)
    else()
        target_link_libraries(leuko_lib PRIVATE leuko_prism_helpers prism_static)
    endif()
else()
    if(TARGET yaml)
        target_link_libraries(leuko_lib PRIVATE leuko_prism_helpers yaml)
    else()
        target_link_libraries(leuko_lib PRIVATE leuko_prism_helpers)
    endif()
endif()

# Link libuv if available
if(TARGET libuv::libuv)
    # Check that the imported target actually has a valid IMPORTED_LOCATION. When using
    # ExternalProject the installed artifacts may not exist at configure time and CMake
    # can create an imported target without a real location which causes generated Makefiles
    # to include a dependency like "libuv::libuv-NOTFOUND" and break the build. Defer
    # linking until the imported location is present.
    get_target_property(_libuv_loc libuv::libuv IMPORTED_LOCATION)
    if(_libuv_loc)
        target_link_libraries(leuko_lib PRIVATE libuv::libuv)
        target_compile_definitions(leuko_lib PRIVATE LEUKO_HAVE_LIBUV=1)
    else()
        message(STATUS "libuv imported target present but no IMPORTED_LOCATION yet; deferring linking until build-time (ExternalProject)")
    endif()
endif()

# Basic testing enablement
enable_testing()

# Ensure Prism external build runs before main targets, if configured
if(TARGET prism_project)
    if(TARGET leuko)
        add_dependencies(leuko prism_project)
    endif()
    if(TARGET leuko_lib)
        add_dependencies(leuko_lib prism_project)
    endif()
endif()
