###############################################################################
# Find Klee
###############################################################################

find_package(KLEE REQUIRED)

if (KLEE_FOUND)
    message(STATUS "Found KLEE")
    message(STATUS "KLEE_INCLUDE_DIRS: ${KLEE_INCLUDE_DIRS}")
    message(STATUS "KLEE_LIBRARY_DIRS: ${KLEE_LIBRARY_DIRS}")
    message(STATUS "KLEE_LIBRARIES: ${KLEE_LIBRARIES}")
else()
    message (FATAL_ERROR "KLEE not found.")
endif()
