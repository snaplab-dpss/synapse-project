################################################################################
# Execution Plan library
################################################################################

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
list(FILTER SOURCES EXCLUDE REGEX ".*/Templates/.*$")

add_library(Synapse SHARED ${SOURCES})
set_target_properties(Synapse PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(Synapse PUBLIC Core)
target_link_libraries(Synapse PUBLIC BDD)

target_link_options(Core PRIVATE "-Wl,--whole-archive")
target_link_libraries(Synapse PRIVATE tomlplusplus::tomlplusplus)
target_link_libraries(Synapse PRIVATE ${GUROBI_LIBRARIES})
target_include_directories(Synapse PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../)
target_include_directories(Synapse PRIVATE ${GUROBI_INCLUDE_DIRS})
target_link_options(Core PRIVATE "-Wl,--no-whole-archive")

target_compile_options(Synapse PRIVATE ${SYNAPSE_COMPILATION_FLAGS})
