cmake_minimum_required(VERSION 3.15)
project(netcache-server)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT DEFINED ENV{RTE_SDK})
    message(FATAL_ERROR "RTE_SDK is not set")
endif()

set(PROG "server")

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBDPDK REQUIRED libdpdk)

set(RTE_SDK "$ENV{RTE_SDK}")

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

set(CFLAGS "-std=c++11 -DSDE_INSTALL=\"${SDE_INSTALL}\" -DRTE_TARGET=\"${RTE_SDK}\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CFLAGS}")

set(LDFLAGS "-Wl,-rpath,${RTE_SDK}/build/lib")
set(LDLIBS "-L${RTE_TARGET}/lib -W")

add_executable(server
	${CMAKE_CURRENT_SOURCE_DIR}/src/server.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/store.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/conf.cpp
)

target_compile_options(${PROG} PRIVATE ${LIBDPDK_CFLAGS})
target_link_libraries(${PROG} ${LDFLAGS} ${LDLIBS} ${LIBDPDK_LDFLAGS})

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
