# LibYAML helper: require libyaml (system or vendor)
# Option to enable building libyaml from vendor (enabled by default)
option(BUILD_LIBYAML "Build vendor/libyaml" ON)

# Try pkg-config for libyaml
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(LIBYAML libyaml QUIET)
endif()

if(LIBYAML_FOUND)
    message(STATUS "Found libyaml via pkg-config: ${LIBYAML_VERSION}")
    add_library(libyaml::yaml UNKNOWN IMPORTED)
    set_target_properties(libyaml::yaml PROPERTIES
        IMPORTED_LOCATION "${LIBYAML_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${LIBYAML_INCLUDE_DIRS}")
    set(LEUKO_HAVE_LIBYAML TRUE)
elseif(EXISTS ${CMAKE_SOURCE_DIR}/vendor/libyaml/CMakeLists.txt)
    message(STATUS "Building vendor/libyaml as part of the project")
    add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/libyaml ${CMAKE_BINARY_DIR}/vendor/libyaml)
    if(TARGET yaml)
        add_library(libyaml::yaml ALIAS yaml)
        set(LEUKO_HAVE_LIBYAML TRUE)
    endif()
else()
    message(STATUS "Attempting to download and build libyaml via ExternalProject")
    include(ExternalProject)
    set(_libyaml_install_dir ${CMAKE_BINARY_DIR}/vendor/libyaml/install)
    set(LEUKO_LIBYAML_INSTALL_DIR ${_libyaml_install_dir} CACHE PATH "libyaml install dir")
    ExternalProject_Add(libyaml_ep
        GIT_REPOSITORY https://github.com/yaml/libyaml.git
        GIT_TAG 0.2.5
        PREFIX ${CMAKE_BINARY_DIR}/vendor/libyaml
        CONFIGURE_COMMAND ${CMAKE_COMMAND} -S <SOURCE_DIR> -B <BINARY_DIR> -DCMAKE_INSTALL_PREFIX=${_libyaml_install_dir}
        BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config ${CMAKE_BUILD_TYPE}
        INSTALL_COMMAND ${CMAKE_COMMAND} --install <BINARY_DIR> --config ${CMAKE_BUILD_TYPE}
        BUILD_BYPRODUCTS ${_libyaml_install_dir}/lib/libyaml.a ${_libyaml_install_dir}/lib/libyaml.so
    )
    add_library(libyaml::yaml STATIC IMPORTED)
    if(EXISTS "${_libyaml_install_dir}/lib/libyaml.a")
        set_target_properties(libyaml::yaml PROPERTIES IMPORTED_LOCATION "${_libyaml_install_dir}/lib/libyaml.a")
    elseif(EXISTS "${_libyaml_install_dir}/lib/libyaml.so")
        set_target_properties(libyaml::yaml PROPERTIES IMPORTED_LOCATION "${_libyaml_install_dir}/lib/libyaml.so")
    endif()
    set(LEUKO_HAVE_LIBYAML TRUE)
endif()

if(NOT DEFINED LEUKO_HAVE_LIBYAML)
    message(FATAL_ERROR "libyaml support is required but libyaml was not found or could not be configured. Set BUILD_LIBYAML=ON and ensure vendor/libyaml is available or pkg-config is installed.")
endif()
