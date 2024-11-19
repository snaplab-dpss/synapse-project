cmake_minimum_required(VERSION 3.15)
project(netcache-server)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(server
	${CMAKE_CURRENT_SOURCE_DIR}/src/server.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/listener.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/store.cpp
)

target_link_libraries(server)
