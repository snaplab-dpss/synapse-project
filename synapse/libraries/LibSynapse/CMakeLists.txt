################################################################################
# Execution Plan library
################################################################################

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
list(FILTER SOURCES EXCLUDE REGEX ".*/Templates/.*$")

add_library(Synapse STATIC ${SOURCES})

target_include_directories(Synapse PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../)

target_link_libraries(Synapse PUBLIC Core)
target_link_libraries(Synapse PUBLIC BDD)
