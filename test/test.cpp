
#define CATCH_CONFIG_MAIN

#include "../lib/Catch2/single_include/catch2/catch.hpp"
#include "../cpp/parse.hpp"

using namespace Catch;

TEST_CASE( "Should match lines", "[match]" ) {
    auto root = parse_source( "../samples/simple.rcl" );
    REQUIRE( root != nullptr );
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
    auto root = parse_source( "../samples/simple.rcl" );
    REQUIRE( root != nullptr );
    Node * line = nullptr;
    const auto & on_line = [&]( Node * node ) {
        if ( node->type == TYPE::LINE && node->source_pos.line == 2 ) {
            line = node;
            return false;
        }
        return true;
    };
    pulse( root, on_line );
    REQUIRE( line != nullptr );
    set< int32_t > cols;
    for ( const auto & ref : line->refs ) {
        if ( ref->type == TYPE::OPERATOR )
            cols.insert( ref->source_pos.char_start );
    }
    REQUIRE( cols.size() == 3 );
}

TEST_CASE( "Should parse terms last line symbols as well", "[match]" ) {
    auto root = parse_source( "../samples/program_1.rcl" );
    REQUIRE( root != nullptr );
    set< string > terms;
    const auto & on_term = [&]( Node * node ) {
        if ( node->type == TYPE::TERM )
            terms.insert( node->content );
        return true;
    };
    pulse( root, on_term );
    REQUIRE( terms.contains( "345" ) );
    REQUIRE( terms.contains( "12345" ) );
    REQUIRE( terms.contains( "4" ) );
}

