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

# Include required helpers (Prism, LibUV, CJSON are required for this project)
include(Prism)
include(LibUV)
include(CJSON)

# Automatically collect .c sources under src/
file(GLOB_RECURSE LEUKO_SOURCES_REL "${CMAKE_SOURCE_DIR}/src/*.c")
# Exclude main.c from library sources (it is used for the executable)
list(REMOVE_ITEM LEUKO_SOURCES_REL "${CMAKE_SOURCE_DIR}/src/main.c")

# Remove sources that depend on deleted/removed headers so the project can
# build in a restructured/reduced state (non-destructive; we do not restore
# deleted files here).
foreach(_s IN LISTS LEUKO_SOURCES_REL)
    if(_s MATCHES "/src/configs/" OR _s MATCHES "/src/rules/" OR _s MATCHES "/src/sources/")
        list(REMOVE_ITEM LEUKO_SOURCES_REL "${_s}")
    endif()
endforeach()

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

# Main executable (if present and required CLI sources are available)
# Main executable (if present and parser.c is compiled into sources)
if(EXISTS ${CMAKE_SOURCE_DIR}/src/main.c)
    list(FIND LEUKO_SOURCES_REL "${CMAKE_SOURCE_DIR}/src/cli/parser.c" _parser_idx)
    if(_parser_idx GREATER -1)
        add_executable(leuko ${CMAKE_SOURCE_DIR}/src/main.c)
        # Link order chosen to match previous setup
        if(TARGET prism_static)
            target_link_libraries(leuko PRIVATE prism_static leuko_lib pthread)
        else()
            target_link_libraries(leuko PRIVATE leuko_lib pthread)
        endif()
    else()
        message(STATUS "Skipping building 'leuko' executable: parser.c not included in sources")
    endif()
endif()

# If libuv is being built via ExternalProject, ensure library target depends on it
if(TARGET libuv_ep)
    add_dependencies(leuko_lib libuv_ep)
endif()

# Link libraries for library target (conditionally add prism/yaml)
if(TARGET prism_static)
    target_link_libraries(leuko_lib PRIVATE leuko_prism_helpers prism_static)
    target_compile_definitions(leuko_lib PRIVATE LEUKO_HAVE_PRISM=1)
else()
    target_link_libraries(leuko_lib PRIVATE leuko_prism_helpers)
endif()

# Link cJSON if available and set compile-time flag
if(TARGET cJSON)
    target_link_libraries(leuko_lib PRIVATE cJSON)
    target_compile_definitions(leuko_lib PRIVATE LEUKO_HAVE_CJSON=1)
elseif(TARGET cjson)
    target_link_libraries(leuko_lib PRIVATE cjson)
    target_compile_definitions(leuko_lib PRIVATE LEUKO_HAVE_CJSON=1)
endif()

# If vendor cJSON directory exists but its include dir isn't exposed by the target,
# add the vendored path as a fallback so <cJSON.h> works during compilation.
if(EXISTS ${CMAKE_SOURCE_DIR}/vendor/cjson)
    target_include_directories(leuko_lib PRIVATE ${CMAKE_SOURCE_DIR}/vendor/cjson)
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

# If libuv was built/installed via ExternalProject, add include dir and static link as fallback
if(DEFINED LEUKO_LIBUV_INSTALL_DIR)
    if(EXISTS ${LEUKO_LIBUV_INSTALL_DIR}/include)
        target_include_directories(leuko_lib PRIVATE ${LEUKO_LIBUV_INSTALL_DIR}/include)
    endif()
    if(EXISTS ${LEUKO_LIBUV_INSTALL_DIR}/lib/libuv.a)
        add_library(libuv_from_ep STATIC IMPORTED)
        set_target_properties(libuv_from_ep PROPERTIES IMPORTED_LOCATION ${LEUKO_LIBUV_INSTALL_DIR}/lib/libuv.a)
        target_link_libraries(leuko_lib PRIVATE libuv_from_ep)
        target_compile_definitions(leuko_lib PRIVATE LEUKO_HAVE_LIBUV=1)
    endif()
    if(TARGET libuv_ep)
        add_dependencies(leuko_lib libuv_ep)
    endif()
endif()

# If cJSON was built via ExternalProject, expose include and link it
if(DEFINED LEUKO_CJSON_INSTALL_DIR)
    if(EXISTS ${LEUKO_CJSON_INSTALL_DIR}/include)
        target_include_directories(leuko_lib PUBLIC ${LEUKO_CJSON_INSTALL_DIR}/include)
    endif()
    if(EXISTS ${LEUKO_CJSON_INSTALL_DIR}/lib/libcjson.a)
        add_library(cjson_from_ep STATIC IMPORTED)
        set_target_properties(cjson_from_ep PROPERTIES IMPORTED_LOCATION ${LEUKO_CJSON_INSTALL_DIR}/lib/libcjson.a)
        target_link_libraries(leuko_lib PRIVATE cjson_from_ep)
    endif()
    if(TARGET cjson_ep)
        add_dependencies(leuko_lib cjson_ep)
    endif()
    target_compile_definitions(leuko_lib PUBLIC LEUKO_HAVE_CJSON=1)
endif()

# If Prism was built via ExternalProject, expose include dir and ensure dependency
if(DEFINED LEUKO_PRISM_INCLUDE_DIR)
    target_include_directories(leuko_lib PRIVATE ${LEUKO_PRISM_INCLUDE_DIR})
    include_directories(${LEUKO_PRISM_INCLUDE_DIR})
endif()
if(TARGET prism_project)
    add_dependencies(leuko_lib prism_project)
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
