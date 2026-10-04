###############################################################################
# Find gurobi
###############################################################################

# Whether the placer is built on Gurobi or on Z3 is decided here, at configure time. Gurobi
# needs a license to start, so AUTO takes it only when the package and a license file are both
# there (a build with Gurobi and no license cannot place anything).
set(ENABLE_GUROBI "AUTO" CACHE STRING "Build the placer on Gurobi: ON, OFF or AUTO (when found and licensed)")

set(_gurobi_license "")
if (DEFINED ENV{GRB_LICENSE_FILE} AND EXISTS "$ENV{GRB_LICENSE_FILE}")
    set(_gurobi_license "$ENV{GRB_LICENSE_FILE}")
elseif (EXISTS "$ENV{HOME}/gurobi.lic")
    set(_gurobi_license "$ENV{HOME}/gurobi.lic")
elseif (DEFINED ENV{GUROBI_HOME} AND EXISTS "$ENV{GUROBI_HOME}/../gurobi.lic")
    set(_gurobi_license "$ENV{GUROBI_HOME}/../gurobi.lic")
elseif (EXISTS "${EXTERNAL_DEPS_DIR}/gurobi1300/gurobi.lic")
    set(_gurobi_license "${EXTERNAL_DEPS_DIR}/gurobi1300/gurobi.lic")
endif()

if (NOT ENABLE_GUROBI STREQUAL "OFF")
    find_package(GUROBI)
endif()

if (ENABLE_GUROBI STREQUAL "OFF")
    message(STATUS "Gurobi disabled: the placer solves with Z3")
elseif (NOT GUROBI_FOUND)
    if (ENABLE_GUROBI STREQUAL "ON")
        message(FATAL_ERROR "ENABLE_GUROBI=ON but GUROBI was not found (GUROBI_HOME: ${GUROBI_HOME})")
    endif()
    message(STATUS "GUROBI not found: the placer solves with Z3")
elseif (ENABLE_GUROBI STREQUAL "AUTO" AND _gurobi_license STREQUAL "")
    message(STATUS "GUROBI found but no license (GRB_LICENSE_FILE, ~/gurobi.lic, ${GUROBI_HOME}/../gurobi.lic): the placer solves with Z3; -DENABLE_GUROBI=ON overrides")
    set(GUROBI_FOUND FALSE)
else()
    message(STATUS "Found GUROBI (license: ${_gurobi_license})")
    message(STATUS "GUROBI_INCLUDE_DIRS: ${GUROBI_INCLUDE_DIRS}")
    message(STATUS "GUROBI_LIBRARIES: ${GUROBI_LIBRARIES}")
    add_definitions(-DUSE_GUROBI_SOLVER)
endif()
