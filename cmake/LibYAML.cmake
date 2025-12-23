# LibYAML helper: optionally add vendor/libyaml as a subdirectory
#
# Option to enable building libyaml from vendor (disabled by default)
option(BUILD_LIBYAML "Build vendor/libyaml" OFF)

if(BUILD_LIBYAML AND EXISTS ${CMAKE_SOURCE_DIR}/vendor/libyaml/CMakeLists.txt)
    add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/libyaml ${CMAKE_BINARY_DIR}/vendor/libyaml)
else()
    message(STATUS "BUILD_LIBYAML is OFF or vendor/libyaml missing; skipping libyaml build")
endif()
