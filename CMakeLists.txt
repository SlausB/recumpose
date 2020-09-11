cmake_minimum_required(VERSION 3.0.0)
project(recumpose VERSION 0.1.0)


include_directories( ~/boost_1_72_0 )
SET(GCC_COVERAGE_COMPILE_FLAGS "-std=gnu++2a -fconcepts -Wall -fmax-errors=4    -mfpmath=both -march=native -m64 -mavx2")
add_definitions(${GCC_COVERAGE_COMPILE_FLAGS})


include(CTest)
enable_testing()

file( GLOB_RECURSE SOURCES
    cpp/main.cpp
)

add_executable( recumpose ${SOURCES} )

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
