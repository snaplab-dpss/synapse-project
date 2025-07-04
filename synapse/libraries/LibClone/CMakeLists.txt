################################################################################
# Clone library
################################################################################

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(Clone SHARED ${SOURCES})
set_target_properties(Clone PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(Clone PUBLIC Core)
target_link_libraries(Clone PUBLIC BDD)

target_link_options(Core PRIVATE "-Wl,--whole-archive")
target_include_directories(Clone PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../)
target_link_options(Core PRIVATE "-Wl,--no-whole-archive")

target_compile_options(Clone PRIVATE ${GLOBAL_COMPILATION_FLAGS})
