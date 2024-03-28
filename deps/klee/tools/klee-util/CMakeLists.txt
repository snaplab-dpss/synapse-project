#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
file(GLOB_RECURSE klee-util-sources
  "*.cpp"
)

file(GLOB_RECURSE load-call-paths-sources
  "../load-call-paths/*.cpp"
)

list(FILTER load-call-paths-sources EXCLUDE REGEX ".*main\\.cpp$")

add_executable(klee-util
  ${klee-util-sources}
  ${load-call-paths-sources}
)

set(KLEE_LIBS
  kleaverExpr
  kleeCore
)

target_include_directories(klee-util PRIVATE ../load-call-paths)
target_link_libraries(klee-util ${KLEE_LIBS})

install(TARGETS klee-util RUNTIME DESTINATION bin)
