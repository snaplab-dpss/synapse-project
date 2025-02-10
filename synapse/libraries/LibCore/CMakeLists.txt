################################################################################
# Core library
################################################################################

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(Core STATIC ${SOURCES})

target_include_directories(Core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../)
target_include_directories(Core PUBLIC ${Z3_INCLUDE_DIRS})
target_include_directories(Core PUBLIC ${KLEE_INCLUDES})
target_include_directories(Core PUBLIC ${LLVM_INCLUDE_DIRS})
target_include_directories(Core PUBLIC ${PCAP_INCLUDE_DIR})

target_compile_definitions(Core PRIVATE ${LLVM_DEFINITIONS})
target_compile_definitions(Core PRIVATE ${KLEE_DEFINITIONS})

target_link_libraries(Core PRIVATE ${KLEE_LIBRARIES})
target_link_libraries(Core PRIVATE ${LLVM_LIBRARIES})
target_link_libraries(Core PRIVATE ${Z3_LIBRARIES})
target_link_libraries(Core PRIVATE ${CURSES_LIBRARIES})
target_link_libraries(Core PRIVATE ${PCAP_LIBRARY})
target_link_libraries(Core PRIVATE Threads::Threads)
target_link_libraries(Core PRIVATE ${CMAKE_DL_LIBS})
