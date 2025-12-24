# Tries to find an install of the Gurobi library and header files
#
# Once done this will define
#  GUROBI_FOUND
#  GUROBI_INCLUDE_DIRS
#  GUROBI_LIBRARIES

if (DEFINED ENV{GUROBI_HOME} AND NOT "$ENV{GUROBI_HOME}" STREQUAL "")
    set(GUROBI_HOME "$ENV{GUROBI_HOME}")
    message(STATUS "GUROBI_HOME: $ENV{GUROBI_HOME}")
else()
    set(_DEFAULT_GUROBI_HOME "${EXTERNAL_DEPS_DIR}/gurobi1300/linux64")

    if (EXISTS "${_DEFAULT_GUROBI_HOME}")
        set(GUROBI_HOME "${_DEFAULT_GUROBI_HOME}")
        message(STATUS "GUROBI_HOME not set; using bundled GUROBI at ${GUROBI_HOME}")
    else()
        message(FATAL_ERROR "GUROBI_HOME is not set. Set GUROBI_HOME, export GUROBI_HOME, or place GUROBI at ${_DEFAULT_GUROBI_HOME}.")
    endif()
endif()

find_path(GUROBI_INCLUDE_DIRS
    NAMES gurobi_c++.h
    HINTS ${GUROBI_HOME}
    PATH_SUFFIXES include)

find_library(GUROBI_C_LIBRARY
    NAMES gurobi120 gurobi130
    HINTS ${GUROBI_HOME}
    PATH_SUFFIXES lib)

find_library(GUROBI_CXX_LIBRARY
    NAMES gurobi_c++
    HINTS ${GUROBI_HOME}
    PATH_SUFFIXES lib)

set(GUROBI_LIBRARIES
    ${GUROBI_CXX_LIBRARY}
    ${GUROBI_C_LIBRARY}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GUROBI DEFAULT_MSG GUROBI_INCLUDE_DIRS GUROBI_LIBRARIES)

