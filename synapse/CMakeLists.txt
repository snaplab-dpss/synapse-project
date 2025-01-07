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
    -Wall
    -Wextra
    -Werror
    -Wfatal-errors
    -Wno-unused-parameter
    -Wnon-virtual-dtor
    -pedantic
    -Wcast-align
    -Wmisleading-indentation
    -Wduplicated-cond
    -Wduplicated-branches
    -Wlogical-op
    -Wdouble-promotion
    -fno-omit-frame-pointer
)

# Uncomment the following lines to enable AddressSanitizer
# add_compile_options(-fsanitize=address)
# add_link_options(-fsanitize=address)

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

###############################################################################
# Build the synapse library
###############################################################################

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)
list(FILTER SOURCES EXCLUDE REGEX ".*/templates/.*$")

file(GLOB_RECURSE TOOLS ${PROJECT_SOURCE_DIR}/tools/*.cpp)

add_library(synapse ${SOURCES})

target_compile_definitions(synapse PRIVATE ${LLVM_DEFINITIONS})
target_compile_definitions(synapse PRIVATE ${KLEE_DEFINITIONS})

target_link_libraries(synapse PUBLIC ${KLEE_LIBRARIES})
target_link_libraries(synapse PUBLIC ${LLVM_LIBRARIES})
target_link_libraries(synapse PUBLIC ${CURSES_LIBRARIES})
target_link_libraries(synapse PUBLIC ${Z3_LIBRARIES})
target_link_libraries(synapse PUBLIC ${LIBUNWIND_LIBRARIES})
target_link_libraries(synapse PUBLIC CLI11::CLI11)
target_link_libraries(synapse PUBLIC nlohmann_json::nlohmann_json)
target_link_libraries(synapse PUBLIC tomlplusplus::tomlplusplus)
target_link_libraries(synapse PUBLIC ${PCAP_LIBRARY})
target_link_libraries(synapse PUBLIC Threads::Threads)
target_link_libraries(synapse PUBLIC dl)

###############################################################################
# Build executables (synapse and tools)
###############################################################################

foreach(TOOL ${TOOLS})
    get_filename_component(TOOL_NAME ${TOOL} NAME_WE)

    set(TOOL_TARGET "${TOOL_NAME}-target")
    set(TOOL_SOURCES ${TOOL})

    add_executable(${TOOL_TARGET} ${TOOL_SOURCES})
    set_target_properties(${TOOL_TARGET} PROPERTIES OUTPUT_NAME "${TOOL_NAME}")
    
    target_link_libraries(${TOOL_TARGET} PUBLIC synapse)
endforeach(TOOL ${TOOLS})


