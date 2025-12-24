# Tries to find an install of the KLEE library and header files
#
# Once done this will define
#  KLEE_FOUND
#  KLEE_INCLUDE_DIRS
#  KLEE_LIBRARY_NAMES    # List of full paths to the found libraries
#  KLEE_LIBRARY_NAMES    # List of short names (e.g., "kleeCore")
#  KLEE_LIBRARY_DIRS     # List of directories where KLEE libraries were found

if (DEFINED ENV{KLEE_DIR} AND NOT "$ENV{KLEE_DIR}" STREQUAL "")
    set(KLEE_DIR "$ENV{KLEE_DIR}")
    message(STATUS "KLEE_DIR: $ENV{KLEE_DIR}")
else()
    set(_DEFAULT_KLEE_DIR "${EXTERNAL_DEPS_DIR}/klee")

    if (EXISTS "${_DEFAULT_KLEE_DIR}")
        set(KLEE_DIR "${_DEFAULT_KLEE_DIR}")
        message(STATUS "KLEE_DIR not set; using bundled KLEE at ${KLEE_DIR}")
    else()
        message(FATAL_ERROR "KLEE_DIR is not set. Set KLEE_DIR, export KLEE_DIR, or place KLEE at ${_DEFAULT_KLEE_DIR}.")
    endif()
endif()

set(KLEE_INCLUDE_DIRS ${KLEE_DIR}/include ${KLEE_DIR}/build/include)
set(KLEE_LIBRARY_DIRS ${KLEE_DIR}/build/lib)

set(KLEE_LIBRARY_NAMES
    kleaverExpr
    kleaverSolver
    kleeBasic
    kleeCore
    kleeModule
    kleeSupport

    # For dealing with cycles in the dependency graph, we need to link again...
    kleaverExpr
)

set(KLEE_LIBRARIES "")

foreach(LIBRARY_NAME ${KLEE_LIBRARY_NAMES})
    find_library(${LIBRARY_NAME}_LIB
        NAMES ${LIBRARY_NAME}
        HINTS ${KLEE_LIBRARY_DIRS})
    if(NOT ${LIBRARY_NAME}_LIB)
        message(FATAL_ERROR "KLEE library ${LIBRARY_NAME} not found in ${KLEE_LIBRARY_DIRS}")
    endif()
    message(STATUS "Found KLEE library: ${${LIBRARY_NAME}_LIB}")
    list(APPEND KLEE_LIBRARIES "${${LIBRARY_NAME}_LIB}")
endforeach()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(KLEE DEFAULT_MSG
    KLEE_INCLUDE_DIRS
    KLEE_LIBRARY_DIRS
    KLEE_LIBRARY_NAMES
)
