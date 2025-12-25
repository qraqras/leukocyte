# CJSON.cmake - locate or fetch cJSON
# Usage: include(CJSON)

include(FetchContent)

# Try to find installed cJSON (CMake package or pkg-config)
find_package(cjson CONFIG QUIET)
if(TARGET cJSON)
  message(STATUS "Found cJSON package via Config")
  return()
endif()

# Try pkg-config
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(CJSON_PKG cjson QUIET)
  if(CJSON_PKG_FOUND)
    # Create imported target
    add_library(cjson INTERFACE IMPORTED)
    target_include_directories(cjson INTERFACE ${CJSON_PKG_INCLUDE_DIRS})
    target_compile_options(cjson INTERFACE ${CJSON_PKG_CFLAGS_OTHER})
    target_link_libraries(cjson INTERFACE ${CJSON_PKG_LIBRARIES})
    message(STATUS "Found cjson via pkg-config: ${CJSON_PKG_LIBRARIES}")
    return()
  endif()
endif()

# Fallback: FetchContent to download cJSON and add as subdirectory
message(STATUS "Fetching cJSON via FetchContent")
FetchContent_Declare(
  cjson
  GIT_REPOSITORY https://github.com/DaveGamble/cJSON.git
  GIT_TAG v1.7.14
)
FetchContent_MakeAvailable(cjson)
# At least the upstream cJSON defines target cJSON
if(TARGET cJSON)
  message(STATUS "Added cJSON via FetchContent")
else()
  message(WARNING "Could not setup cJSON target; proceed without cJSON")
endif()
