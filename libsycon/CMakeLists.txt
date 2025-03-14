cmake_minimum_required(VERSION 3.16)

project(sycon)

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

###############################################################################
# Setting output targets
###############################################################################

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

################################################################################
# Sanity check - Disallow building in source.
# Otherwise we would overwrite the Makefiles of the old build system.
################################################################################

if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In source builds are not allowed. You should invoke "
            "CMake from a different directory.")
endif()

################################################################################
# Add our CMake module directory to the list of module search directories
################################################################################

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

###############################################################################
# Building the sycon library
###############################################################################

# Add the letter "d" to the end of the library name when building in debug mode.
set(CMAKE_DEBUG_POSTFIX d)

file(GLOB SOURCES
    ${PROJECT_SOURCE_DIR}/src/*.c ${PROJECT_SOURCE_DIR}/src/*.cpp
    ${PROJECT_SOURCE_DIR}/src/tables/*.c ${PROJECT_SOURCE_DIR}/src/metatables/*.cpp
    ${PROJECT_SOURCE_DIR}/src/tables/*.c ${PROJECT_SOURCE_DIR}/src/primitives/*.cpp
)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_include_directories(${PROJECT_NAME}
    PUBLIC 
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>    
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
)

set_target_properties(${PROJECT_NAME}
    PROPERTIES
        COMPILE_FLAGS "-mcrc32"
        LINK_FLAGS "-mcrc32"
)

###############################################################################
# Configure external projects
###############################################################################

include(${CMAKE_SOURCE_DIR}/cmake/find_cli11.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/find_pcap.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/find_sde.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/find_absl.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/find_threads.cmake)

SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        CLI11::CLI11
    PRIVATE
        Threads::Threads
        absl::synchronization
        ${PCAP_LIBRARIES}
        ${SDE_LIBRARIES}

        # This fixes the requirement for LD_LIBRARY_PATH update when running the final binaries.
        # https://stackoverflow.com/questions/58997230/cmake-project-fails-to-find-shared-library
        "-Wl,--disable-new-dtags"

        # For some reason some systems need this (it may be specific to g++8).
        "stdc++fs"
)

###############################################################################
# Library installation
###############################################################################

install(
    TARGETS ${PROJECT_NAME}
    EXPORT  ${PROJECT_NAME}_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
    DIRECTORY       ${CMAKE_SOURCE_DIR}/include/
    DESTINATION     include
    FILES_MATCHING  PATTERN "*.h*"
)

install(
    DIRECTORY       ${cli11_SOURCE_DIR}/include/
    DESTINATION     include
    FILES_MATCHING  PATTERN "*.h*"
)

###############################################################################
# Build an executable for each example
###############################################################################

file(GLOB SOURCES_EXAMPLES ${PROJECT_SOURCE_DIR}/examples/*.cpp)

foreach(EXAMPLE_SOURCE ${SOURCES_EXAMPLES})
    get_filename_component(EXAMPLE ${EXAMPLE_SOURCE} NAME_WE)
    add_executable(${EXAMPLE} ${EXAMPLE_SOURCE})
    target_link_libraries(${EXAMPLE} PRIVATE ${PROJECT_NAME})
endforeach(EXAMPLE_SOURCE ${SOURCES_EXAMPLES})
