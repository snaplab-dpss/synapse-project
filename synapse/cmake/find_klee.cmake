###############################################################################
# Find Klee
###############################################################################

if(DEFINED ENV{KLEE_DIR})
    message(STATUS "KLEE_DIR: $ENV{KLEE_DIR}")
    set(KLEE_DIR $ENV{KLEE_DIR})
else()
    message(FATAL_ERROR "KLEE_DIR env var is not set")
endif()

set(KLEE_INCLUDES ${KLEE_DIR}/include ${KLEE_DIR}/build/include)
set(KLEE_LIB_DIR ${KLEE_DIR}/build/lib)

set(KLEE_LIBRARIES
    ${KLEE_LIB_DIR}/libkleeModule.a
    ${KLEE_LIB_DIR}/libkleaverSolver.a
    ${KLEE_LIB_DIR}/libkleeBasic.a
    ${KLEE_LIB_DIR}/libkleaverExpr.a
    ${KLEE_LIB_DIR}/libkleeSupport.a
    ${KLEE_LIB_DIR}/libkleeCore.a
)

message(STATUS "KLEE_LIBRARIES: ${KLEE_LIBRARIES}")