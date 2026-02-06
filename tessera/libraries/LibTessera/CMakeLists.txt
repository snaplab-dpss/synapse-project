################################################################################
# Execution Plan library
################################################################################

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
list(FILTER SOURCES EXCLUDE REGEX ".*/Templates/.*$")

add_library(Tessera SHARED ${SOURCES})
set_target_properties(Tessera PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(Tessera PUBLIC Core)
target_link_libraries(Tessera PUBLIC BDD)

target_link_options(Core PRIVATE "-Wl,--whole-archive")
target_link_libraries(Tessera PRIVATE tomlplusplus::tomlplusplus)
target_link_libraries(Tessera PRIVATE ${GUROBI_LIBRARIES})
target_include_directories(Tessera PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../)
target_include_directories(Tessera PRIVATE ${GUROBI_INCLUDE_DIRS})
target_link_options(Core PRIVATE "-Wl,--no-whole-archive")

target_compile_options(Tessera PRIVATE ${GLOBAL_COMPILATION_FLAGS})
