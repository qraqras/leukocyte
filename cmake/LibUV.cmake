# LibUV helper: use system libuv or build vendor/libuv
# Option to enable libuv support (enabled by default and required)
option(BUILD_LIBUV "Enable libuv support (use system libuv or vendor/libuv)" ON)

if(BUILD_LIBUV)
    # Prefer building the vendored libuv if present to guarantee a consistent libuv
    if(EXISTS ${CMAKE_SOURCE_DIR}/vendor/libuv/CMakeLists.txt)
        message(STATUS "Building vendor/libuv as part of the project (preferred)")
        add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/libuv ${CMAKE_BINARY_DIR}/vendor/libuv)
        # vendor target is usually 'uv' - create an alias if present
        if(TARGET uv)
            add_library(libuv::libuv ALIAS uv)
            set(LEUKO_HAVE_LIBUV TRUE)
        else()
            message(WARNING "vendor/libuv present but expected target 'uv' not found")
        endif()
    else()
        # Try pkg-config first
        find_package(PkgConfig QUIET)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(LIBUV libuv QUIET)
        endif()

        if(LIBUV_FOUND)
            message(STATUS "Found libuv via pkg-config: ${LIBUV_VERSION}")
            add_library(libuv::libuv UNKNOWN IMPORTED)
            set_target_properties(libuv::libuv PROPERTIES
                IMPORTED_LOCATION "${LIBUV_LIBRARIES}"
                INTERFACE_INCLUDE_DIRECTORIES "${LIBUV_INCLUDE_DIRS}")
            set(LEUKO_HAVE_LIBUV TRUE)
        else()
            message(STATUS "Attempting to download and build libuv via ExternalProject")
            include(ExternalProject)
            set(_libuv_install_dir ${CMAKE_BINARY_DIR}/vendor/libuv/install)
            # Expose install dir so other CMakeLists can add include dirs and depend on the ExternalProject
            set(LEUKO_LIBUV_INSTALL_DIR ${_libuv_install_dir} CACHE PATH "libuv install dir")
            ExternalProject_Add(libuv_ep
                GIT_REPOSITORY https://github.com/libuv/libuv.git
                GIT_TAG v1.46.0
                PREFIX ${CMAKE_BINARY_DIR}/vendor/libuv
                CONFIGURE_COMMAND ${CMAKE_COMMAND} -S <SOURCE_DIR> -B <BINARY_DIR> -DCMAKE_INSTALL_PREFIX=${_libuv_install_dir}
                BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config ${CMAKE_BUILD_TYPE}
                INSTALL_COMMAND ${CMAKE_COMMAND} --install <BINARY_DIR> --config ${CMAKE_BUILD_TYPE}
                BUILD_BYPRODUCTS ${_libuv_install_dir}/lib/libuv.a ${_libuv_install_dir}/lib/libuv.so ${_libuv_install_dir}/bin/uv.dll
            )
            # Create imported target pointing to the installed library location (assume static lib by default)
            add_library(libuv::libuv STATIC IMPORTED)
            # Prefer static library but accept shared if present
            if(EXISTS "${_libuv_install_dir}/lib/libuv.a")
                set_target_properties(libuv::libuv PROPERTIES IMPORTED_LOCATION "${_libuv_install_dir}/lib/libuv.a")
            elseif(EXISTS "${_libuv_install_dir}/lib/libuv.so")
                set_target_properties(libuv::libuv PROPERTIES IMPORTED_LOCATION "${_libuv_install_dir}/lib/libuv.so")
            elseif(EXISTS "${_libuv_install_dir}/lib/uv.dll")
                set_target_properties(libuv::libuv PROPERTIES IMPORTED_LOCATION "${_libuv_install_dir}/lib/uv.dll")
            endif()
            # Note: include dir will be available after install; linking target depends on external project
            set(LEUKO_HAVE_LIBUV TRUE)
        endif()
    endif()

    if(NOT DEFINED LEUKO_HAVE_LIBUV)
        message(FATAL_ERROR "libuv support is required (BUILD_LIBUV=ON) but libuv was not found or could not be configured")
    endif()
else()
    message(FATAL_ERROR "libuv support is required for this project. Set BUILD_LIBUV=ON to enable it.")
endif()
