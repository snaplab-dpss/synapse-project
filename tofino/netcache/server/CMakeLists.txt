cmake_minimum_required(VERSION 3.15)
project(netcache-server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_C_EXTENSIONS True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PROG "server")

include(${CMAKE_SOURCE_DIR}/cmake/find_dpdk.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/find_cli11.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/find_json.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/find_libnf.cmake)

add_executable(${PROG}
	${CMAKE_CURRENT_SOURCE_DIR}/src/server.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/store.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/clock.cpp
)

target_compile_options(${PROG} PUBLIC ${DPDK_CFLAGS_OTHER})

target_include_directories(${PROG} PUBLIC ${DPDK_INCLUDE_DIRS})
target_include_directories(${PROG} PUBLIC ${CLI11_INCLUDE_DIRS})
target_include_directories(${PROG} PUBLIC ${JSON_INCLUDE_DIRS})

target_link_directories(${PROG} PUBLIC ${DPDK_LIBRARY_DIRS})

# https://stackoverflow.com/questions/56381463/executable-missing-full-library-path-after-build
# cmake black magic...
target_link_options(${PROG} PUBLIC "-Wl,--disable-new-dtags")

target_link_libraries(${PROG} PUBLIC DPDK::DPDK)
target_link_libraries(${PROG} PUBLIC CLI11::CLI11)
target_link_libraries(${PROG} PUBLIC nlohmann_json::nlohmann_json)