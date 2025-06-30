################################################################################
# Core library
################################################################################

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(Core SHARED ${SOURCES})
set_target_properties(Core PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(Core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../)
target_include_directories(Core SYSTEM PUBLIC ${Z3_INCLUDE_DIRS})
target_include_directories(Core SYSTEM PUBLIC ${KLEE_INCLUDE_DIRS})
target_include_directories(Core SYSTEM PUBLIC ${LLVM_INCLUDE_DIRS})
target_include_directories(Core SYSTEM PUBLIC ${PCAP_INCLUDE_DIR})

target_compile_definitions(Core PRIVATE ${LLVM_DEFINITIONS})

target_link_options(Core PRIVATE "-Wl,--whole-archive")
target_link_libraries(Core PRIVATE ${KLEE_LIBRARIES})
target_link_libraries(Core PRIVATE ${LLVM_LIBRARIES})
target_link_libraries(Core PRIVATE ${Z3_LIBRARIES})
target_link_libraries(Core PRIVATE ${CURSES_LIBRARIES})
target_link_libraries(Core PRIVATE Threads::Threads)
target_link_libraries(Core PRIVATE ${CMAKE_DL_LIBS})
target_link_libraries(Core PRIVATE ZLIB::ZLIB)
target_link_options(Core PRIVATE "-Wl,--no-whole-archive")

target_link_libraries(Core PUBLIC ${PCAP_LIBRARY})

target_compile_options(Core PRIVATE ${SYNAPSE_COMPILATION_FLAGS})
