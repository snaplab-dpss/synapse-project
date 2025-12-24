###############################################################################
# Find gurobi
###############################################################################

find_package(GUROBI)

if (GUROBI_FOUND)
    message(STATUS "Found GUROBI")
    message(STATUS "GUROBI_INCLUDE_DIRS: ${GUROBI_INCLUDE_DIRS}")
    message(STATUS "GUROBI_LIBRARIES: ${GUROBI_LIBRARIES}")
    add_definitions(-DUSE_GUROBI_SOLVER)
else()
    message(STATUS "GUROBI not found in ${GUROBI_HOME}")
endif()

