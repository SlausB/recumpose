
#define CATCH_CONFIG_MAIN

#include "../lib/Catch2/single_include/catch2/catch.hpp"
#include "../cpp/parse.hpp"

using namespace Catch;

unsigned int Factorial( unsigned int number ) {
    return number <= 1 ? number : Factorial(number-1)*number;
}

TEST_CASE( "Factorials are computed", "[factorial]" ) {
    REQUIRE( Factorial(1) == 1 );
    REQUIRE( Factorial(2) == 2 );
    REQUIRE( Factorial(3) == 6 );
    REQUIRE( Factorial(10) == 3628800 );
}

const string SIMPLE = R"V0G0N(
k = 100 = l * 3
x = 2
y = 3
b = 4
)V0G0N";

TEST_CASE( "Should match lines", "[match]" ) {
    auto root = match_lines( SIMPLE );
    match_operators( root );
    print_lines( root );
    int32_t lines_count = 0;
    const auto & on_node = [&]( auto node ) {
        if ( node->type == TYPE::LINE )
            ++ lines_count;
        return true;
    };
    pulse( root, on_node );
    REQUIRE( lines_count == 4 );
}

TEST_CASE( "Should match multiple operators within single line", "[match]" ) {
    auto root = match_lines( SIMPLE );
    match_operators( root );
}

