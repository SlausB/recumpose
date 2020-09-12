# Add catch as an interface library
set( CATCH_INCLUDE_DIR ../lib/Catch2 )
add_library( Catch INTERFACE )
target_include_directories( Catch INTERFACE ${CATCH_INCLUDE_DIR} )

# Add test executable
add_executable( tests test.cpp )
target_link_libraries( tests PUBLIC Catch )

if( TARGET tests )
    enable_testing()
endif()

add_test( NAME tests COMMAND tests )

include(CTest)
# catch_discover_tests(tests)
