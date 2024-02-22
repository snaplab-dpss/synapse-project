###############################################################################
# Find SDE
###############################################################################

find_package(SDE)

if (SDE_FOUND)
    message(STATUS "Found SDE libraries")
    target_include_directories(${PROJECT_NAME} PUBLIC ${SDE_INCLUDE_DIR})
else()
    message(FATAL_ERROR "SDE libraries not found")
endif()