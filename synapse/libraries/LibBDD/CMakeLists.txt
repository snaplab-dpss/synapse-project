################################################################################
# BDD library
################################################################################

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
list(FILTER SOURCES EXCLUDE REGEX ".*/Templates/.*$")

add_library(BDD SHARED ${SOURCES})
set_target_properties(BDD PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(BDD PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../)

target_link_libraries(BDD PUBLIC Core)
target_link_libraries(BDD PUBLIC nlohmann_json::nlohmann_json)

target_link_options(BDD PRIVATE "-Wl,--whole-archive")
target_link_libraries(BDD PRIVATE ${LLVM_LIBRARIES})
target_link_options(BDD PRIVATE "-Wl,--no-whole-archive")

target_compile_options(BDD PRIVATE ${SYNAPSE_COMPILATION_FLAGS})