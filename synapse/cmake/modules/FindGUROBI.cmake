# Tries to find an install of the Gurobi library and header files
#
# Once done this will define
#  GUROBI_FOUND
#  GUROBI_INCLUDE_DIRS
#  GUROBI_LIBRARIES

find_path(GUROBI_INCLUDE_DIRS
    NAMES gurobi_c++.h
    HINTS $ENV{GUROBI_HOME}
    PATH_SUFFIXES include)

find_library(GUROBI_C_LIBRARY
    NAMES gurobi120 gurobi130
    HINTS $ENV{GUROBI_HOME}
    PATH_SUFFIXES lib)

find_library(GUROBI_CXX_LIBRARY
    NAMES gurobi_c++
    HINTS $ENV{GUROBI_HOME}
    PATH_SUFFIXES lib)

set(GUROBI_LIBRARIES
    ${GUROBI_CXX_LIBRARY}
    ${GUROBI_C_LIBRARY}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GUROBI DEFAULT_MSG GUROBI_INCLUDE_DIRS GUROBI_LIBRARIES)

