cmake_minimum_required(VERSION 3.0.0)
project(recumpose VERSION 0.1.0)

include_directories( ~/boost_1_72_0 )
SET(GCC_COVERAGE_COMPILE_FLAGS "-std=gnu++2a -fconcepts -Wall -fmax-errors=4    -mfpmath=both -march=native -m64 -mavx2")
add_definitions(${GCC_COVERAGE_COMPILE_FLAGS})

add_executable( recumpose cpp/main.cpp )
target_link_libraries( recumpose common )

add_subdirectory( test )

add_library( common cpp/parse.hpp )
set_target_properties( common PROPERTIES LINKER_LANGUAGE CXX )

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
