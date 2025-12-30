# cmake/

This directory contains CMake helper modules used by the project.

- `LeukoHelpers.cmake`: small helpers that set compile flags (ASAN, gprof) and add vendor include paths.

Usage:

In `CMakeLists.txt`:

```
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(LeukoHelpers OPTIONAL)
```

Add additional FindXXX.cmake or toolchain files here as needed.

This project now prefers `cJSON` for JSON parsing and no longer requires `libyaml` as a build dependency; a `CJSON.cmake` helper is provided to find or fetch cJSON.
