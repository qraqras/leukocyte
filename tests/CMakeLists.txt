# tests/CMakeLists.txt
# Minimal CMake for tests
# Build test using the leuko_glob implementation directly to avoid requiring external deps
enable_testing()
add_executable(test_glob ${CMAKE_SOURCE_DIR}/tests/test_glob.c ${CMAKE_SOURCE_DIR}/src/sources/leuko_glob.c ${CMAKE_SOURCE_DIR}/src/leuko_debug.c)
target_include_directories(test_glob PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/include/sources)
# If libuv ExternalProject will install headers, add that include dir
if(DEFINED LEUKO_LIBUV_INSTALL_DIR)
    target_include_directories(test_glob PRIVATE ${LEUKO_LIBUV_INSTALL_DIR}/include)
    # Ensure test waits for libuv to be configured and installed by ExternalProject
    add_dependencies(test_glob libuv_ep)
endif()
# Link with libuv (imported target) so the uv symbols resolve when available
# Link with libuv (imported target) if available
if(TARGET libuv::libuv)
    target_link_libraries(test_glob PRIVATE libuv::libuv)
endif()
# If libuv was installed via ExternalProject, link directly against the installed library
if(DEFINED LEUKO_LIBUV_INSTALL_DIR)
    if(EXISTS ${LEUKO_LIBUV_INSTALL_DIR}/lib/libuv.a)
        target_link_libraries(test_glob PRIVATE ${LEUKO_LIBUV_INSTALL_DIR}/lib/libuv.a)
    elseif(EXISTS ${LEUKO_LIBUV_INSTALL_DIR}/lib/libuv.so)
        target_link_libraries(test_glob PRIVATE ${LEUKO_LIBUV_INSTALL_DIR}/lib/libuv.so)
    endif()
endif()
# The project requires libuv; ensure compile-time macro is defined for the test as well
target_compile_definitions(test_glob PRIVATE LEUKO_HAVE_LIBUV=1)
add_test(NAME test_glob COMMAND test_glob)

# Test for Ruby file collector
add_executable(test_file_collector ${CMAKE_SOURCE_DIR}/tests/test_file_collector.c)
target_include_directories(test_file_collector PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/include/sources)
target_link_libraries(test_file_collector PRIVATE leuko_lib)
add_test(NAME test_file_collector COMMAND test_file_collector)

# Test for parallel glob walker
add_executable(test_glob_parallel ${CMAKE_SOURCE_DIR}/tests/test_glob_parallel.c)
target_include_directories(test_glob_parallel PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_glob_parallel PRIVATE leuko_lib)
add_test(NAME test_glob_parallel COMMAND test_glob_parallel)
