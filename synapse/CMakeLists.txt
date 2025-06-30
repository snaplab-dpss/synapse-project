###############################################################################
# Project version
###############################################################################

cmake_minimum_required(VERSION 3.16)

project(
    synapse
    VERSION 1.0
    LANGUAGES C CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

###############################################################################
# Configuring C/C++ standard versions
###############################################################################

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_C_EXTENSIONS True)

###############################################################################
# Enable asserts in RelWithDebInfo mode
###############################################################################

string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")

###############################################################################
# Setting some complation flags
###############################################################################

set(SYNAPSE_COMPILATION_FLAGS
    -Wall
    -Wextra
    -Werror
    -Wfatal-errors
    -Wno-unused-parameter
    -Wnon-virtual-dtor
    -pedantic
    -Wcast-align
    -Wmisleading-indentation
    -Wdouble-promotion
    -fno-omit-frame-pointer
    -Wshadow
)

if (ENABLE_ADDRESS_SANITIZER)
    message(STATUS "WARNING: Address sanitizer enabled")
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

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

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In source builds are not allowed. You should invoke "
            "CMake from a different directory.")
endif()

################################################################################
# Add our CMake module directory to the list of module search directories
################################################################################

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

###############################################################################
# Configure external projects
###############################################################################

include(${CMAKE_SOURCE_DIR}/cmake/find_cli11.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/find_json.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/find_tomlplusplus.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/find_pcap.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/find_z3.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/find_llvm.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/find_klee.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/find_curses.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/find_threads.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/find_unwind.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/find_zlib.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/find_gurobi.cmake)

###############################################################################
# Build the libraries
###############################################################################

add_subdirectory(libraries/LibCore)
add_subdirectory(libraries/LibBDD)
add_subdirectory(libraries/LibSynapse)

###############################################################################
# Build executables
###############################################################################

file(GLOB_RECURSE TOOLS ${PROJECT_SOURCE_DIR}/tools/*.cpp)

foreach(TOOL ${TOOLS})
    get_filename_component(TOOL_NAME ${TOOL} NAME_WE)

    set(TOOL_TARGET "${TOOL_NAME}-target")
    set(TOOL_SOURCES ${TOOL})

    add_executable(${TOOL_TARGET} ${TOOL_SOURCES})
    set_target_properties(${TOOL_TARGET} PROPERTIES OUTPUT_NAME "${TOOL_NAME}")
    
    target_link_libraries(${TOOL_TARGET} PUBLIC Core)
    target_link_libraries(${TOOL_TARGET} PUBLIC BDD)
    target_link_libraries(${TOOL_TARGET} PUBLIC Synapse)
    target_link_libraries(${TOOL_TARGET} PUBLIC CLI11::CLI11)
endforeach(TOOL ${TOOLS})
