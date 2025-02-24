cmake_minimum_required(VERSION 3.15)
project(netcache-server)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PROG "server")

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBDPDK REQUIRED libdpdk)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

# PC_FILE := $(shell $(PKGCONF) --path libdpdk 2>/dev/null)
# CFLAGS  += $(shell $(PKGCONF) --cflags libdpdk)
# LDFLAGS  = $(shell $(PKGCONF) --static --libs libdpdk)
# LDFLAGS += -L$(RTE_SDK)/$(RTE_TARGET)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CFLAGS}")

add_executable(server
	${CMAKE_CURRENT_SOURCE_DIR}/src/server.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/store.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/conf.cpp
)

target_compile_options(${PROG} PRIVATE ${LIBDPDK_CFLAGS})
target_link_libraries(${PROG} ${LDFLAGS} ${LDLIBS} ${LIBDPDK_LDFLAGS})

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
