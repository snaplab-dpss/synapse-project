cmake_minimum_required(VERSION 3.15)
project(netcache-controller)

if(NOT DEFINED ENV{SDE_INSTALL})
    message(FATAL_ERROR "SDE_INSTALL is not set")
endif()

# Set variables
set(P4_PROG "netcache-controller")
set(ARCH "tofino2")
set(CUR_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(SRC_DIR "${CUR_DIR}/src")
set(OUT_DIR "${CUR_DIR}/build")

# Include directories
set(SDE_INSTALL "$ENV{SDE_INSTALL}")
include_directories(${SDE_INSTALL}/include)
include_directories(${SDE_INSTALL}/include/${ARCH}/pdfixed)

set(CMAKE_CXX_STANDARD 11)

file(GLOB_RECURSE SOURCES "${SRC_DIR}/*.cpp")
file(GLOB_RECURSE HEADERS "${SRC_DIR}/*.h")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUT_DIR})

set(CFLAGS "-std=c++11 -DSDE_INSTALL=\"${SDE_INSTALL}\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CFLAGS}")

set(BF_LIBS "-ldriver")
set(LDFLAGS "-Wl,-rpath,${SDE_INSTALL}/lib")
set(LDLIBS "-L${SDE_INSTALL}/lib -Wl,--start-group ${BF_LIBS} -Wl,--end-group -lm -pthread -lpcap -ldl -levent")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${P4_PROG} ${SOURCES})

target_link_libraries(${P4_PROG} ${LDFLAGS} ${LDLIBS})

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
