set(SDE_INCLUDE_DIR $ENV{SDE_INSTALL}/include)
set(SDE_LIBRARIES_DIR $ENV{SDE_INSTALL}/lib)

set(SDE_LIBRARIES "")

file(GLOB ALL_SDE_LIBRARIES "${SDE_LIBRARIES_DIR}/lib*\.so*")

file(GLOB EXCLUDED_SDE_LIBRARIES
    "${SDE_LIBRARIES_DIR}/*avago*"
    "${SDE_LIBRARIES_DIR}/*gitref*"
    "${SDE_LIBRARIES_DIR}/*unsecure*"
)

# Avoid using the list FILTER feature, as it's only present on cmake 3.6 onwards.
list(REMOVE_ITEM ALL_SDE_LIBRARIES ${EXCLUDED_SDE_LIBRARIES})

foreach(LIB ${ALL_SDE_LIBRARIES})
    get_filename_component(LIB_NAME ${LIB} NAME)

    string(REGEX MATCH "lib(.+)\.so.*" _ ${LIB_NAME})
    set(LINKABLE_LIB_NAME ${CMAKE_MATCH_1})

    # Force the find_library to find the new library,
    # otherwise it always returns the same lib (the first one).
    SET(FOUND_LIB "FOUND_LIB-NOTFOUND")
    
    find_library(FOUND_LIB
        NAMES ${LIB_NAME}
        REQUIRED
        HINTS ${SDE_LIBRARIES_DIR}
    )
    
    if(${FOUND_LIB} STREQUAL "FOUND_LIB-NOTFOUND")
        message(FATAL_ERROR "Failed to find ${LIB_NAME}")
    endif()

    list(APPEND SDE_LIBRARIES ${FOUND_LIB})
endforeach()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SDE DEFAULT_MSG
    SDE_LIBRARIES
    SDE_INCLUDE_DIR
)

include(CheckCSourceCompiles)
set(CMAKE_REQUIRED_LIBRARIES ${SDE_LIBRARIES})

mark_as_advanced(
    SDE_INCLUDE_DIR
    SDE_LIBRARIES
)
