# Prism helper: require Prism (vendor) and create an imported target
# Option to enable building Prism via ExternalProject (enabled by default)
option(BUILD_PRISM "Build vendor/prism using ExternalProject" ON)

# Prefer a pre-built libprism.a if present (common during local development)
if(EXISTS ${CMAKE_SOURCE_DIR}/vendor/prism/build/libprism.a)
    add_library(prism_static STATIC IMPORTED GLOBAL)
    set(PRISM_LIB_PATH ${CMAKE_SOURCE_DIR}/vendor/prism/build/libprism.a)
    set_target_properties(prism_static PROPERTIES
        IMPORTED_LOCATION ${PRISM_LIB_PATH}
        INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/vendor/prism/include
    )
    set(LEUKO_HAVE_PRISM TRUE)
else()
    # Prefer an initialized vendor/prism with source files (Makefile/README/src)
    if(BUILD_PRISM AND EXISTS ${CMAKE_SOURCE_DIR}/vendor/prism AND (EXISTS ${CMAKE_SOURCE_DIR}/vendor/prism/Makefile OR EXISTS ${CMAKE_SOURCE_DIR}/vendor/prism/README.md OR EXISTS ${CMAKE_SOURCE_DIR}/vendor/prism/src))
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
        set(LEUKO_HAVE_PRISM TRUE)
    else()
        # Vendor prism missing or not initialized: clone and build via ExternalProject
        include(ExternalProject)
        set(_prism_install_dir ${CMAKE_BINARY_DIR}/vendor/prism/install)
        set(PRISM_GIT_URL "https://github.com/ruby/prism.git")
        ExternalProject_Add(prism_project
            GIT_REPOSITORY ${PRISM_GIT_URL}
            GIT_TAG main
            PREFIX ${CMAKE_BINARY_DIR}/vendor/prism
            CONFIGURE_COMMAND ""
            BUILD_COMMAND cd <SOURCE_DIR> && export PKG_CONFIG_PATH=${LEUKO_LIBYAML_INSTALL_DIR}/lib/pkgconfig:$${PKG_CONFIG_PATH} && gem install bundler --no-document || true && bundle config build.psych --with-libyaml-dir=${LEUKO_LIBYAML_INSTALL_DIR} && bundle install --jobs 4 --retry 3 && bundle exec rake make
            BUILD_IN_SOURCE 1
            INSTALL_COMMAND ""
            BUILD_BYPRODUCTS <SOURCE_DIR>/build/libprism.a <SOURCE_DIR>/build/libprism${CMAKE_SHARED_LIBRARY_SUFFIX}
            DEPENDS libyaml_ep
        )

        add_library(prism_static STATIC IMPORTED GLOBAL)
        # Imported location may appear after the ExternalProject build; prefer the built location under the ExternalProject source dir
        if(EXISTS ${CMAKE_BINARY_DIR}/vendor/prism/src/prism_project/build/libprism.a)
            set(PRISM_LIB_PATH ${CMAKE_BINARY_DIR}/vendor/prism/src/prism_project/build/libprism.a)
        elseif(EXISTS ${CMAKE_BINARY_DIR}/vendor/prism/src/prism_project/build/libprism${CMAKE_SHARED_LIBRARY_SUFFIX})
            set(PRISM_LIB_PATH ${CMAKE_BINARY_DIR}/vendor/prism/src/prism_project/build/libprism${CMAKE_SHARED_LIBRARY_SUFFIX})
        else()
            # Fallback to expected source dir path
            set(PRISM_LIB_PATH ${CMAKE_SOURCE_DIR}/vendor/prism/build/libprism.a)
        endif()
        set_target_properties(prism_static PROPERTIES
            IMPORTED_LOCATION ${PRISM_LIB_PATH}
        )
        # Export include dir variable for consumers to use; do NOT set INTERFACE_INCLUDE_DIRECTORIES
        set(LEUKO_PRISM_INCLUDE_DIR ${CMAKE_BINARY_DIR}/vendor/prism/src/prism_project/include CACHE PATH "Prism include dir (may be available after ExternalProject builds)" FORCE)
        if(EXISTS ${LEUKO_PRISM_INCLUDE_DIR})
            include_directories(${LEUKO_PRISM_INCLUDE_DIR})
        endif()
        set(LEUKO_HAVE_PRISM TRUE)
    endif()
endif()
