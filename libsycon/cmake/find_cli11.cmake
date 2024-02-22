###############################################################################
# Find CLI11
###############################################################################

include(FetchContent)

FetchContent_Declare(
    cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG        v2.3.2
)

FetchContent_GetProperties(cli11)

if(NOT cli11_POPULATED)
    FetchContent_Populate(cli11)

    add_subdirectory(
        ${cli11_SOURCE_DIR}
        ${cli11_BINARY_DIR}
    )
endif()

