cmake_minimum_required(VERSION 3.15)
project(netcache-server)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PROG "server")

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBDPDK REQUIRED libdpdk)

add_executable(server
	${CMAKE_CURRENT_SOURCE_DIR}/src/server.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/store.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/conf.cpp
)

target_compile_options(${PROG} PRIVATE ${LIBDPDK_CFLAGS})
target_link_libraries(${PROG} ${LIBDPDK_STATIC_LDFLAGS})