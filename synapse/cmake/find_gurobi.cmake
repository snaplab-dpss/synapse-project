###############################################################################
# Find gurobi
###############################################################################

if(DEFINED ENV{GUROBI_HOME})
    message(STATUS "GUROBI_HOME: $ENV{GUROBI_HOME}")
    set(GUROBI_HOME $ENV{GUROBI_HOME})
else()
    message(FATAL_ERROR "GUROBI_HOME env var is not set")
endif()

find_package(GUROBI)

if (GUROBI_FOUND)
    message(STATUS "Found GUROBI")
    message(STATUS "GUROBI_INCLUDE_DIRS: ${GUROBI_INCLUDE_DIRS}")
    message(STATUS "GUROBI_LIBRARIES: ${GUROBI_LIBRARIES}")
    add_definitions(-DUSE_GUROBI_SOLVER)
else()
    message (FATAL_ERROR "Gurobi not found.")
endif()
