# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/workspaces/leukocyte/vendor/prism"
  "/workspaces/leukocyte/build_gprof/prism_project-prefix/src/prism_project-build"
  "/workspaces/leukocyte/build_gprof/prism_project-prefix"
  "/workspaces/leukocyte/build_gprof/prism_project-prefix/tmp"
  "/workspaces/leukocyte/build_gprof/prism_project-prefix/src/prism_project-stamp"
  "/workspaces/leukocyte/build_gprof/prism_project-prefix/src"
  "/workspaces/leukocyte/build_gprof/prism_project-prefix/src/prism_project-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/workspaces/leukocyte/build_gprof/prism_project-prefix/src/prism_project-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/workspaces/leukocyte/build_gprof/prism_project-prefix/src/prism_project-stamp${cfgdir}") # cfgdir has leading slash
endif()
