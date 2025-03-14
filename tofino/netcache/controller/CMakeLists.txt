cmake_minimum_required(VERSION 3.15)
project(netcache-controller)

###############################################################################
# Configuring C++ standard version
###############################################################################

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

###############################################################################
# Configuring C standard version
###############################################################################

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_C_EXTENSIONS True)

###############################################################################
# We need to know where SDE is installed
###############################################################################

IF(DEFINED ENV{SDE_INSTALL})
    MESSAGE(STATUS "SDE_INSTALL: $ENV{SDE_INSTALL}")
ELSE()
    MESSAGE(FATAL_ERROR "SDE_INSTALL env var is not set")
ENDIF()

###############################################################################
# Setting some complation flags
###############################################################################

add_compile_options(
    -Wall -Wextra -Werror -Wfatal-errors
    -Wno-unused-variable -Wno-unused-parameter
)

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

################################################################################
# Add our CMake module directory to the list of module search directories
################################################################################

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

################################################################################
# Set some variables
################################################################################

set(PROG "netcache-controller")
set(ARCH "tofino2")
set(CUR_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(SRC_DIR "${CUR_DIR}/src")

###############################################################################
# Setting output targets
###############################################################################

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

file(GLOB_RECURSE SOURCES "${SRC_DIR}/*.cpp")
file(GLOB_RECURSE HEADERS "${SRC_DIR}/*.h")

add_executable(${PROG} ${SOURCES})

###############################################################################
# Configure external projects
###############################################################################

include(${CMAKE_SOURCE_DIR}/cmake/find_cli11.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/find_sde.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/find_absl.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/find_threads.cmake)

target_link_libraries(${PROG}
    PUBLIC
        CLI11::CLI11
    PRIVATE
        Threads::Threads
        absl::synchronization
        ${SDE_LIBRARIES}

        # This fixes the requirement for LD_LIBRARY_PATH update when running the final binaries.
        # https://stackoverflow.com/questions/58997230/cmake-project-fails-to-find-shared-library
        "-Wl,--disable-new-dtags"

        # For some reason some systems need this (it may be specific to g++8).
        "stdc++fs"
)
