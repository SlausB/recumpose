
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <list>
#include <queue>
#include <set>
#include <cctype>
#include <utility>
#include <algorithm>
#include "samples.hpp"
#include "syntax_tree.hpp"
#include "plot.hpp"

using namespace std;

Node * match_lines( const auto & input ) {
    Node * start = nullptr;
    //currently matching:
    Node * last = nullptr;

    //first break into lines:
    int32_t line_number = 1;
    int32_t line_char_number = 0;
    int32_t line_start = 0;
    int32_t cur_pos = 0;
    auto caret = input.begin();
    while ( caret != input.end() ) {
        if ( * caret == '\n' ) {
            //line with some content:
            if ( cur_pos - line_start > 0 ) {
                //append new semantic node which represents the parsed line:
                auto new_one = new Node(
                    input.substr( line_start, cur_pos - line_start ),
                    TYPE::LINE,
                    SourcePos(
                        line_number,
                        1,
                        //-1 because chars (col) counting in text editors starts from 1 instead of 0 :(
                        line_char_number - 1
                    )
                );
                if ( start == nullptr ) {
                    start = new_one;
                    last = new_one;
                }
                else {
                    last->ref( new_one );
                    last = new_one;
                }
            }
            line_start = cur_pos + 1;
            line_char_number = 0;
            ++ line_number;
        }

        ++ caret;
        ++ cur_pos;
        ++ line_char_number;
    }

    return start;
}

/** Spawn CHAR Nodes for every character of specified Operator and connect it with both OPERATOR and LINE Nodes.*/
void spawn_op_chars(
    const auto & op,
    Node * op_node,
    Node * line_node
) {
    int32_t disp = 0;
    for ( char ch : op ) {
        auto ch_node = new Node(
            string( 1, ch ),
            TYPE::CHAR,
            op_node->source_pos.disp( disp, 1 )
        );
        op_node->ref( ch_node );
        line_node->ref( ch_node );
        ++ disp;
    }
}

bool try_match_operator(
    Node * line_node,
    const string & op,
    size_t & caret
) {
    const auto r = line_node->content.find( op, caret );

    if ( r == string::npos )
        return false;
    
    auto op_node = new Node(
        op,
        TYPE::OPERATOR,
        line_node->source_pos.disp( r, op.size() )
    );
    cout << "Operator found: " << op << " at " << op_node->source_pos << endl;
    line_node->ref( op_node );
    spawn_op_chars( op, op_node, line_node );
    caret = r + op.size();
    return true;
}

void match_operators( Node * root ) {
    const auto & on_line = []( Node * line_node ){
        if ( line_node->type != TYPE::LINE )
            return true;
        
        size_t caret = 0;
        while ( caret < line_node->content.size() ) {
            bool found = false;
            for ( const auto & op : Operators ) {
                while ( caret < line_node->content.size() ) {
                    found = try_match_operator( line_node, op, caret );
                    if ( ! found )
                        break;
                }
            }
            if ( ! found )
                break;
        }

        return true;
    };
    pulse( root, on_line );
}

/** Remove operators which consist of literals (like "if","then"...) and are parts of terms.*/
void fix_literal_ops( Node * root ) {
    set< Node * > remove;

    const auto & on_op = [&]( Node * op_node ) {
        if ( op_node->type != TYPE::OPERATOR )
            return true;
        
        auto line_node = closest( op_node, TYPE::LINE );
        if ( line_node == nullptr )
            return true;
        
        for ( const auto ch : op_node->content ) {
            if ( ! isalpha( ch ) )
                return true;
        }
        
        if (
            //at left:
            (
                op_node->source_pos.char_start > line_node->source_pos.char_start
                &&
                isalpha( line_node->content.at( op_node->source_pos.char_start - 1 ) )
            )
            ||
            //at right:
            (
                op_node->source_pos.char_end < line_node->source_pos.char_end
                &&
                isalpha( line_node->content.at( op_node->source_pos.char_end ) )
            )
        ) {
            remove.insert( op_node );
        }

        return true;
    };
    pulse( root, on_op );

    for ( auto & r : remove )
        delete r;
    cout << remove.size() << " literals Operators fixed back to become terms." << endl;
}

void match_terms( Node * root ) {
    const auto & on_line = [&]( Node * line_node ) {
        if ( line_node->type != TYPE::LINE )
            return true;
        
        int32_t seq_start = 0;
        for ( size_t caret = 0; caret <= line_node->content.size(); ++ caret ) {
            if (
                caret >= line_node->content.size()
                ||
                ! isalnum( line_node->content.at( caret ) )
                ||
                //check that not yet an operator:
                check_char(
                    root,
                    TYPE::OPERATOR,
                    line_node->source_pos.disp( caret, 1 )
                )
            ) {
                const auto length = caret - seq_start;
                if ( length > 0 ) {
                    auto term_node = new Node(
                        line_node->content.substr( seq_start, length ),
                        TYPE::TERM,
                        line_node->source_pos.disp( seq_start, length )
                    );
                    term_node->ref( line_node );
                    cout << "Term " << term_node->content << " spawned at " << term_node->source_pos << " with length " << length << endl;
                }
                seq_start = caret + 1;
            }
        }
        return true;
    };
    pulse( root, on_line );
}

auto match_symmetries( Node * root ) {
    const auto & on_equal = [&]( Node * eq_node ) {
        if ( eq_node->type != TYPE::OPERATOR || eq_node->content != "=" )
            return true;
        
        auto line_node = closest( eq_node, TYPE::LINE );
        if ( line_node == nullptr )
            throw runtime_error( "Equal operator without line" );
        
        list< NodeHandler > sorted;
        for ( const auto & ref : line_node->refs ) {
            switch ( ref->type ) {
                case TYPE::TERM:
                case TYPE::OPERATOR:
                    sorted.push_back( NodeHandler( ref ) );
                    break;
                default:
                    break;
            }
        }
        sorted.sort();
        cout << "Line " << line_node->source_pos.line << " has this these TERMs and OPERATORs refs: " << endl;
        for ( const auto & h : sorted )
            cout << "    " << h.node->content << endl;
        return true;
    };
    pulse( root, on_equal );
}

auto parse_source( const auto & input ) {
    auto root = match_lines( input );
    match_operators( root );
    fix_literal_ops( root );
    match_terms( root );
    match_symmetries( root );
    return root;
}

auto parse()
{
    const auto input = PROGRAM_1;
    if ( input.size() <= 0 ) {
        cout << "Empty program source." << endl;
        return;
    }
    
    cout << "Parsing the source:" << endl << input << endl;
    auto root = parse_source( input );

    plot( root );

    cout << "Done." << endl;
}
