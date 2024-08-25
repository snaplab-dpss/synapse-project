###############################################################################
# Project version
###############################################################################

cmake_minimum_required(VERSION 3.16)

project(
    synapse
    VERSION 1.0
    LANGUAGES C CXX
)

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

add_compile_options(
    -Wall -Wextra -Werror -Wfatal-errors
    -Wno-unused-variable -Wno-unused-parameter
)

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
include(${CMAKE_SOURCE_DIR}/cmake/find_pcap.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/find_z3.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/find_llvm.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/find_klee.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/find_curses.cmake)

###############################################################################
# Build executables (synapse and tools)
###############################################################################

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)
list(FILTER SOURCES EXCLUDE REGEX ".*/templates/.*$")

file(GLOB_RECURSE TOOLS ${PROJECT_SOURCE_DIR}/tools/*.cpp)

foreach(TOOL ${TOOLS})
    get_filename_component(TOOL_NAME ${TOOL} NAME_WE)
    set(TOOL_SOURCES ${TOOL} ${SOURCES})

    add_executable(${TOOL_NAME} ${TOOL_SOURCES})

    target_compile_definitions(${TOOL_NAME} PRIVATE ${LLVM_DEFINITIONS})
    target_compile_definitions(${TOOL_NAME} PRIVATE ${KLEE_DEFINITIONS})
    
    target_link_libraries(${TOOL_NAME} PUBLIC ${KLEE_LIBRARIES})
    target_link_libraries(${TOOL_NAME} PUBLIC ${LLVM_LIBRARIES})
    target_link_libraries(${TOOL_NAME} PUBLIC ${CURSES_LIBRARIES})
    target_link_libraries(${TOOL_NAME} PUBLIC ${Z3_LIBRARIES})
    target_link_libraries(${TOOL_NAME} PUBLIC CLI11::CLI11)
    target_link_libraries(${TOOL_NAME} PUBLIC nlohmann_json::nlohmann_json)
    target_link_libraries(${TOOL_NAME} PUBLIC ${PCAP_LIBRARY})
endforeach(TOOL ${TOOLS})


