cmake_minimum_required(VERSION 3.15)
project(netcache-controller)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT DEFINED ENV{SDE_INSTALL})
    message(FATAL_ERROR "SDE_INSTALL is not set")
endif()

# if(NOT DEFINED ENV{RTE_SDK})
#     message(FATAL_ERROR "RTE_SDK is not set")
# endif()

# Set variables
set(PROG "netcache-controller")
set(ARCH "tofino2")
set(CUR_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(SRC_DIR "${CUR_DIR}/src")
set(OUT_DIR "${CUR_DIR}/build")

# find_package(PkgConfig REQUIRED)
# pkg_check_modules(LIBDPDK REQUIRED libdpdk)

# Include directories
set(SDE_INSTALL "$ENV{SDE_INSTALL}")
include_directories(${SDE_INSTALL}/include)
include_directories(${SDE_INSTALL}/include/${ARCH}/pdfixed)

# set(RTE_SDK "$ENV{RTE_SDK}")

set(CMAKE_CXX_STANDARD 11)

file(GLOB_RECURSE SOURCES "${SRC_DIR}/*.cpp")
file(GLOB_RECURSE HEADERS "${SRC_DIR}/*.h")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUT_DIR})

# set(CFLAGS "-std=c++11 -DSDE_INSTALL=\"${SDE_INSTALL}\" -DRTE_TARGET=\"${RTE_SDK}\"")
set(CFLAGS "-std=c++11 -DSDE_INSTALL=\"${SDE_INSTALL}\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CFLAGS}")

set(BF_LIBS "-ldriver -ltarget_sys")
# set(BF_LIBS "-ldriver -lbfsys")
set(LDFLAGS "-Wl,-rpath,${SDE_INSTALL}/lib")
# set(LDFLAGS "-Wl,-rpath,${RTE_SDK}/build/lib")
# set(LDLIBS "-L${SDE_INSTALL}/lib -Wl,-L${RTE_TARGET}/lib -Wl,--start-group ${BF_LIBS} -Wl,--end-group -lm -pthread -lpcap -ldl -levent")
set(LDLIBS "-L${SDE_INSTALL}/lib -Wl,--start-group ${BF_LIBS} -Wl,--end-group -lm -pthread -lpcap -ldl -levent")

add_executable(${PROG} ${SOURCES})

# target_compile_options(${PROG} PRIVATE ${LIBDPDK_CFLAGS})
# target_link_libraries(${PROG} ${LDFLAGS} ${LDLIBS} ${LIBDPDK_LDFLAGS})
target_link_libraries(${PROG} ${LDFLAGS} ${LDLIBS} "-Wl,--disable-new-dtags")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
