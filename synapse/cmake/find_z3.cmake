###############################################################################
# Find Z3
###############################################################################

if(DEFINED ENV{Z3_DIR})
    message(STATUS "Z3_DIR: $ENV{Z3_DIR}")
    set(Z3_DIR $ENV{Z3_DIR})
else()
    message(FATAL_ERROR "Z3_DIR env var is not set")
endif()

find_package(Z3 REQUIRED)

message(STATUS "Found Z3")

set(ENABLE_Z3 1) # For config.h

# Check the signature of `Z3_get_error_msg()`
set(HAVE_Z3_GET_ERROR_MSG_NEEDS_CONTEXT 1)
set (_old_CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES}")
set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ${Z3_LIBRARIES})
check_prototype_definition(Z3_get_error_msg
  "Z3_string Z3_get_error_msg(Z3_context c, Z3_error_code err)"
  "NULL" "${Z3_INCLUDE_DIRS}/z3.h" HAVE_Z3_GET_ERROR_MSG_NEEDS_CONTEXT)
set(CMAKE_REQUIRED_LIBRARIES ${_old_CMAKE_REQUIRED_LIBRARIES})
if (HAVE_Z3_GET_ERROR_MSG_NEEDS_CONTEXT)
  message(STATUS "Z3_get_error_msg requires context")
else()
  message(STATUS "Z3_get_error_msg does not require context")
endif()

message(STATUS "Z3_LIBRARIES: ${Z3_LIBRARIES}")
message(STATUS "Z3_INCLUDE_DIRS: ${Z3_INCLUDE_DIRS}")
