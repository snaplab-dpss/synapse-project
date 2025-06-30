###############################################################################
# Find ZLIB
###############################################################################

find_package(ZLIB REQUIRED)

if (ZLIB_FOUND)
  message(STATUS "Found ZLIB")
else()
  message (FATAL_ERROR "ZLIB not found.")
endif()
