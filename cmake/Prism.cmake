# Prism helper: sets up building or importing the vendor/prism library.
#
# Optionally build Prism via ExternalProject (disabled by default to allow minimal builds)
option(BUILD_PRISM "Build vendor/prism using ExternalProject" OFF)

# Prefer a pre-built libprism.a if present (common during local development)
if(EXISTS ${CMAKE_SOURCE_DIR}/vendor/prism/build/libprism.a)
    add_library(prism_static STATIC IMPORTED GLOBAL)
    set(PRISM_LIB_PATH ${CMAKE_SOURCE_DIR}/vendor/prism/build/libprism.a)
    set_target_properties(prism_static PROPERTIES
        IMPORTED_LOCATION ${PRISM_LIB_PATH}
        INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/vendor/prism/include
    )
else()
    if(BUILD_PRISM AND EXISTS ${CMAKE_SOURCE_DIR}/vendor/prism)
        include(ExternalProject)
        ExternalProject_Add(prism_project
            SOURCE_DIR ${CMAKE_SOURCE_DIR}/vendor/prism
            CONFIGURE_COMMAND ""
            BUILD_COMMAND cd ${CMAKE_SOURCE_DIR}/vendor/prism && env CPPFLAGS="-I${CMAKE_SOURCE_DIR}/include -I${CMAKE_SOURCE_DIR}/include/utils/allocator" CFLAGS="${CFLAGS} -DPRISM_XALLOCATOR" make static
            BUILD_IN_SOURCE 1
            INSTALL_COMMAND ""
        )

        add_library(prism_static STATIC IMPORTED GLOBAL)
        if(EXISTS ${CMAKE_SOURCE_DIR}/vendor/prism/build/libprism.a)
            set(PRISM_LIB_PATH ${CMAKE_SOURCE_DIR}/vendor/prism/build/libprism.a)
        else()
            set(PRISM_LIB_PATH ${CMAKE_SOURCE_DIR}/vendor/prism/build/libprism${CMAKE_SHARED_LIBRARY_SUFFIX})
        endif()
        set_target_properties(prism_static PROPERTIES
            IMPORTED_LOCATION ${PRISM_LIB_PATH}
            INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/vendor/prism/include
        )
    else()
        message(STATUS "BUILD_PRISM is OFF or vendor/prism missing; skipping Prism ExternalProject and imported library")
    endif()
endif()
