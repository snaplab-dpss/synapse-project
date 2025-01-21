################################################################################
# BDD library
################################################################################

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
list(FILTER SOURCES EXCLUDE REGEX ".*/Templates/.*$")

add_library(BDD STATIC ${SOURCES})

target_include_directories(BDD PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../)

target_link_libraries(BDD PUBLIC Core)
target_link_libraries(BDD PUBLIC nlohmann_json::nlohmann_json)
