# Tries to find an install of the Z3 library and header files
#
# Once done this will define
#  Z3_FOUND - BOOL: System has the Z3 library installed
#  Z3_INCLUDE_DIRS - LIST:The GMP include directories
#  Z3_LIBRARIES - LIST:The libraries needed to use Z3
include(FindPackageHandleStandardArgs)

if (DEFINED ENV{Z3_DIR} AND NOT "$ENV{Z3_DIR}" STREQUAL "")
  set(Z3_DIR "$ENV{Z3_DIR}")
  message(STATUS "Z3_DIR: $ENV{Z3_DIR}")
else()
  set(_DEFAULT_Z3_DIR "${EXTERNAL_DEPS_DIR}/z3")
  if (EXISTS "${_DEFAULT_Z3_DIR}")
    set(Z3_DIR "${_DEFAULT_Z3_DIR}")
    message(STATUS "Z3_DIR not set; using bundled Z3 at ${Z3_DIR}")
  else()
    message(FATAL_ERROR "Z3_DIR is not set. Set Z3_DIR, export Z3_DIR, or place Z3 at ${_DEFAULT_Z3_DIR}.")
  endif()
endif()

find_library(Z3_LIBRARIES
  NAMES z3
  DOC "Z3 libraries"
  PATHS "${Z3_DIR}/build"
  NO_DEFAULT_PATH
)

if (Z3_LIBRARIES)
  message(STATUS "Found Z3 libraries: \"${Z3_LIBRARIES}\"")
else()
  message(STATUS "Could not find Z3 libraries")
endif()

# Try to find headers
find_path(Z3_INCLUDE_DIRS
    NAMES z3.h
    # For distributions that keep the header files in a `z3` folder,
    # for example Fedora's `z3-devel` package at `/usr/include/z3/z3.h`
    PATH_SUFFIXES z3
    DOC "Z3 C header"
    PATHS "${Z3_DIR}/build/include"
    NO_DEFAULT_PATH
)
if (Z3_INCLUDE_DIRS)
    message(STATUS "Found Z3 include path: \"${Z3_INCLUDE_DIRS}\"")
else()
    message(STATUS "Could not find Z3 include path")
endif()

# TODO: We should check we can link some simple code against libz3

# Handle QUIET and REQUIRED and check the necessary variables were set and if so
# set ``Z3_FOUND``
find_package_handle_standard_args(Z3 DEFAULT_MSG Z3_INCLUDE_DIRS Z3_LIBRARIES)
