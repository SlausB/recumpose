
#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <memory>
#include <stdexcept>
#include <list>
#include <queue>
#include <set>
#include <cctype>
#include <utility>
#include <algorithm>
#include <numeric>
#include "syntax_tree.hpp"
#include "plot.hpp"

using namespace std;

/** Returns true if specified string consist of whitespace characters only.*/
bool only_whitespace( const string & line ) {
    for ( const auto & ch : line ) {
        if ( ! isspace( ch ) )
            return false;
    }
    return true;
}

Node * parse_lines( const string & file_name ) {
    ifstream file( file_name );

    auto file_node = new Node(
        "",
        TYPE::SOURCE_FILE,
        SourcePos( 0, 0, 0, file_name )
    );

    //chaining:
    auto node = file_node;

    int32_t line_number = 0;
    string line;
    while ( getline( file, line ) ) {
        ++ line_number;

        if ( only_whitespace( line ) )
            continue;

        auto new_one = new Node(
            line,
            TYPE::LINE,
            SourcePos(
                line_number,
                1,
                line.size(),
                file_name
            )
        );
        
        if ( node != nullptr )
            node->ref( new_one );
        node = new_one;
    }

    return file_node;
}

void remove_empty_lines( Node * root ) {
    set< Node * > remove;

    const auto & on_line = [&]( Node * line_node ) {
        if ( line_node->type != TYPE::LINE )
            return true;
        
        if ( only_whitespace( line_node->content ) ) {
            const auto P = { TYPE::LINE, TYPE::SOURCE_FILE };
            auto parent = line_node->parent( P );
            auto child = line_node->child( TYPE::LINE );
            if ( parent && child )
                parent->ref( child );
            remove.insert( line_node );
        }

        return true;
    };
    pulse( root, on_line );

    for ( const auto & r : remove )
        delete r;
}

/** Detects comments and removes their content from lines.*/
void parse_comments( Node * root ) {
    const auto & on_file = [&]( Node * file_node ) {
        if ( file_node->type != TYPE::SOURCE_FILE )
            return true;
        
        //TODO: properly handle comment matchers inside literals (there is no definition for literals yet anyway) ...

        bool in_multiline = false;
        auto node = file_node->child( TYPE::LINE );
        
        while ( node != nullptr ) {
            if ( in_multiline ) {
                const auto multi_end = node->content.find( "*/" );
                if ( multi_end == string::npos ) {
                    //completely remove the line:
                    auto next = node->child( TYPE::LINE );
                    const auto P = { TYPE::LINE, TYPE::SOURCE_FILE };
                    auto parent = node->parent( P );
                    if ( parent && next )
                        parent->ref( next );
                    delete node;
                    node = next;
                    continue;
                }
                else {
                    in_multiline = false;
                    node->source_pos.char_start += multi_end + 2;
                    node->content = node->content.substr( multi_end + 2 );
                }
            }

            const auto single = node->content.find( "//", 0 );
            if ( single != string::npos ) {
                node->source_pos.char_end = node->source_pos.char_start + single;
                node->content = node->content.substr( 0, single );
            }
            else {
                const auto multi = node->content.find( "/*", 0 );
                if ( multi != string::npos ) {
                    const auto multi_end = node->content.find( "*/", multi + 2 );
                    if ( multi_end == string::npos ) {
                        in_multiline = true;
                        node->source_pos.char_end = node->source_pos.char_start + multi;
                        node->content = node->content.substr( 0, multi );
                        //remaining on that same line to search for multiline comment ending:
                        continue;
                    }
                    else {
                        node->content = node->content.replace( multi, multi_end + 2 - multi, " " );
                    }
                }
            }

            node = node->child( TYPE::LINE );
        }
        return true;
    };
    pulse( root, on_file );

    remove_empty_lines( root );
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

/** Returns true if specified string contains of alphabetic characters only.*/
bool is_alpha_string( const string & s ) {
    for ( const auto ch : s ) {
        if ( ! isalpha( ch ) )
            return false;
    }
    return true;
}

bool match_operator(
    Node * line_node,
    const string & op,
    size_t & caret
) {
    const auto r = line_node->content.find( op, caret );

    if ( r == string::npos )
        return false;
    
    //TODO: skip it if matched source range intersect with some other matched element:
    /*for ( auto line_el : line_node->refs ) {
        if ( line_el.type == TYPE::SOURCE_FILE || line_el.type == TYPE::LINE )
            continue;
        if ( line_el->source_pos.intersects( r, op.size() ) )
            return false;
    }*/

    //deny matching if literal operator has other literals around:
    if (
        is_alpha_string( op )
        &&
        (
            //at left:
            (
                r > 0
                &&
                isalpha( line_node->content.at( r - 1 ) )
            )
            ||
            //at right:
            (
                r + op.size() < line_node->content.size()
                &&
                isalpha( line_node->content.at( r + op.size() ) )
            )
        )
    ) {
        return false;
    }
    
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
        
        //TODO: match operators starting from longest to shortest ones:
        for ( const auto & op : Operators ) {
            size_t caret = 0;
            while ( caret < line_node->content.size() ) {
                if ( ! match_operator( line_node, op, caret ) )
                    break;
            }
        }

        return true;
    };
    pulse( root, on_line );
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
                    line_node->ref( term_node );
                    cout << "Term " << term_node->content << " spawned at " << term_node->source_pos << " with length " << length << endl;
                }
                seq_start = caret + 1;
            }
        }
        return true;
    };
    pulse( root, on_line );
}

struct FileCache {
    list< Node * > terms;
    list< Node * > operators;
};

void chain_terms_and_operators(
    Node * root,
    map< Node *, FileCache > & cache
) {
    const auto & on_file = [&]( Node * file_node ) {
        if ( file_node->type != TYPE::SOURCE_FILE )
            return;
        
        //expression that is currently being parsed:
        Node * caret = nullptr;
        
        auto line_node = file_node;
        while ( ( line_node = line_node->child( TYPE::LINE ) ) ) {
            list< Node * > terms_and_ops;

            for ( auto & to_ref : line_node->refs ) {
                if ( to_ref->type == TYPE::TERM || to_ref->type == TYPE::OPERATOR ) {
                    terms_and_ops.push_back( to_ref );
                }
            }

            syntactic_position_sort( terms_and_ops );
            for ( auto & to : terms_and_ops ) {
                if ( to->type == TYPE::TERM )
                    cache[ file_node ].terms.push_back( to );
                if ( to->type == TYPE::OPERATOR )
                    cache[ file_node ].operators.push_back( to );

                if ( caret == nullptr ) {
                    caret = to;
                    file_node->ref( caret );
                }
                else {
                    caret->ref( to );
                    caret = to;
                }
            }
        }
    };
    pulse( root, on_file );
}

int32_t index_in( const auto & array, const auto & el ) {
    int32_t i = -1;
    for ( const auto & existing : array ) {
        ++ i;
        if ( el == existing )
            return i;
    }
    return -1;
}
/** Returns element at specified position within ordered container with random access ability (like list, vector, array, etc.).*/
auto random_access_at( auto & array, const auto & pos ) {
    auto it = begin( array );
    advance( it, pos );
    return * it;
}

/** Returns operators sorted by their semantical precedence.*/
template< typename Ops >
auto semantic_operators_order( const Ops & ops ) {
    //sort operators by their precedence order:
    vector< size_t > pointers( ops.size() );
    iota( pointers.begin(), pointers.end(), 0 );
    stable_sort(
        pointers.begin(),
        pointers.end(),
        [&](
            const size_t & i1,
            const size_t & i2
        ) {
            return
                index_in( Operators, random_access_at( ops, i1 )->content )
                <
                index_in( Operators, random_access_at( ops, i2 )->content )
            ;
        }
    );

    list< typename Ops::value_type > result;
    for ( const auto & p : pointers )
        result.push_back( random_access_at( ops, p ) );
    return result;
}

Node * ultimate_parent_expression( Node * source ) {
    const set< TYPE > pass{ TYPE::LINE, TYPE::SOURCE_FILE };
    const auto & on_parent = [&]( Node * parent ) {
        source = parent;
    };
    pulse< false >( source, on_parent, false, pass );
    return source;
}

auto obtain_relative( Node * op, auto & from, const string & orient ) {
    auto right_term = find_type( from, TYPE::TERM );
    if ( right_term == nullptr ) {
        cout << "ERROR: no term at " << orient << " from operator " << op->content << " at " << op->source_pos << endl;
        throw runtime_error( "semantics error" );
    }
    return ultimate_parent_expression( right_term );
}

auto consume_left( Node * op, Node * expr = nullptr ) {
    auto left = obtain_relative( op, op->refd, "left" );

    if ( expr == nullptr )
        expr = new Node(
            op->content + " expression",
            TYPE::EXPRESSION,
            //TODO: merge sources:
            op->source_pos
        );
    expr->ref( op );
    expr->ref( left );
    return expr;
}
auto consume_right( Node * op, Node * expr = nullptr ) {
    auto right = obtain_relative( op, op->refs, "right" );

    if ( expr == nullptr )
        expr = new Node(
            op->content + " expression",
            TYPE::EXPRESSION,
            //TODO: merge sources:
            op->source_pos
        );
    expr->ref( op );
    expr->ref( right );
    return expr;
}
void consume_infix( Node * op ) {
    auto expr = consume_left( op );
    consume_right( op, expr );
}

void match_semantics(
    map< Node *, FileCache > & cache
) {
    for ( auto & file : cache ) {
        auto ops = semantic_operators_order( file.second.operators );

        cout << "Operators sorted by their precedence:" << endl;
        for ( const auto & op : ops ) {
            cout << "    " << op->content << " at " << op->source_pos << endl;
        }

        //operators consume their operands:
        for ( const auto & op : ops ) {
            switch ( Operands.at( op->content ) ) {
                case OPERAND::INFIX:
                    consume_infix( op );
                    break;
                case OPERAND::LEFT:
                    consume_left( op );
                    break;
                case OPERAND::RIGHT:
                    consume_right( op );
                    break;
            }
        }

        /*//remaining terms are Entities:
        for ( auto entity : file.second.terms ) {
            auto parent_expr = ultimate_parent_expression( entity );
            //if not an entity:
            if ( parent_expr != entity )
                continue;
            consume_right( entity );
        }*/
    }
}

auto parse_source( const string & file_name ) {
    auto root = parse_lines( file_name );
    if ( root == nullptr ) {
        cout << "ERROR: empty source." << endl;
        return root;
    }
    parse_comments( root );
    match_operators( root );
    match_terms( root );

    map< Node *, FileCache > cache;
    chain_terms_and_operators( root, cache );
    match_semantics( cache );

    return root;
}

auto print_symmetries( Node * root ) {
    const auto & on_sym = [&]( Node * sym_node ) {
        if ( sym_node->type != TYPE::EQUALITY )
            return true;
        
        auto line_node = closest( sym_node, TYPE::LINE );
        cout << "Line " << line_node->source_pos.line << " has such symmetries:" << endl;
        for ( const auto & ref : sym_node->refs ) {
            if ( ref->type == TYPE::EXPRESSION )
                cout << "    " << ref->content << endl;
        }
        return true;
    };
    pulse( root, on_sym );
}

void print_file( const string & file_name ) {
    ifstream file( file_name );
    cout << "Source \"" << file_name << "\" input file:" << endl;
    cout << "================================================" << endl;
    string line;
    int32_t line_number = 0;
    while ( getline( file, line ) ) {
        ++ line_number;
        cout << "Line " << line_number << ": \"" << line << "\"" << endl;
    }
    cout << "================================================" << endl;
}

auto parse( const string & file_name )
{
    print_file( file_name );
    auto root = parse_source( file_name );
    if ( root == nullptr )
        return;
    print_lines( root );

    print_symmetries( root );

    plot( root );

    cout << "Done." << endl;
}
