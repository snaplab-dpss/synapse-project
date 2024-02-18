###############################################################################
# Find pcap
###############################################################################

find_package(PCAP)

if (PCAP_FOUND)
    message(STATUS "Found libpcap")
    target_include_directories(${PROJECT_NAME} PUBLIC ${PCAP_INCLUDE_DIR})
else()
    message (FATAL_ERROR "libpcap not found")
endif()
