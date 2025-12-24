###############################################################################
# Find Z3
###############################################################################

find_package(Z3 REQUIRED)

message(STATUS "Found Z3")

set(ENABLE_Z3 1) # For config.h

# Check the signature of `Z3_get_error_msg()`
set(HAVE_Z3_GET_ERROR_MSG_NEEDS_CONTEXT 1)
set (_old_CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES}")
set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ${Z3_LIBRARIES})
set(CMAKE_REQUIRED_LIBRARIES ${_old_CMAKE_REQUIRED_LIBRARIES})
if (HAVE_Z3_GET_ERROR_MSG_NEEDS_CONTEXT)
  message(STATUS "Z3_get_error_msg requires context")
else()
  message(STATUS "Z3_get_error_msg does not require context")
endif()

message(STATUS "Z3_LIBRARIES: ${Z3_LIBRARIES}")
message(STATUS "Z3_INCLUDE_DIRS: ${Z3_INCLUDE_DIRS}")
