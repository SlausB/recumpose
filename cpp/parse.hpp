
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
#include "syntax_tree.hpp"
#include "plot.hpp"

using namespace std;

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
        
        for ( const auto & op : Operators ) {
            size_t caret = 0;
            while ( caret < line_node->content.size() ) {
                if ( ! try_match_operator( line_node, op, caret ) )
                    break;
            }
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

        cout << "Line " << line_node->source_pos.line << " has these TERMs and OPERATORs refs: " << endl;
        for ( const auto & h : sorted )
            cout << "    " << h.node->content << endl;
        
        list< list< Node * > > expressions;
        set< Node * > equality_nodes;
        {
            list< Node * > building_expr;
            for ( auto & h : sorted ) {
                if ( h.node->type == TYPE::OPERATOR && h.node->content == "=" ) {
                    expressions.push_back( building_expr );
                    building_expr.clear();

                    auto equality_node = new Node(
                        "=",
                        TYPE::EQUALITY,
                        h.node->source_pos
                    );
                    equality_nodes.insert( equality_node );
                    line_node->ref( equality_node );
                }
                else {
                    building_expr.push_back( h.node );
                }
            }
            expressions.push_back( building_expr );
        }
        for ( auto & es : expressions ) {
            if ( es.size() < 1 ) {
                cout << "ERROR: expected expressions around \"=\" operator." << endl;
            }
            else {
                auto expr_node = new Node(
                    "",
                    TYPE::EXPRESSION,
                    line_node->source_pos
                );
                expr_node->source_pos.char_start = es.front()->source_pos.char_start;
                expr_node->source_pos.char_start = es.back() ->source_pos.char_end;
                bool first = true;
                for ( const auto & e : es ) {
                    if ( ! first )
                        expr_node->content += " ";
                    first = false;
                    expr_node->content += e->content;
                    expr_node->ref( e );
                }
                line_node->ref( expr_node );

                for ( auto & eq_node : equality_nodes )
                    expr_node->ref( eq_node );
            }
        }

        return true;
    };
    pulse( root, on_equal );
}

auto parse_source( const string & file_name ) {
    auto root = parse_lines( file_name );
    if ( root == nullptr ) {
        cout << "ERROR: empty source." << endl;
        return root;
    }
    print_lines( root );
    parse_comments( root );
    match_operators( root );
    fix_literal_ops( root );
    match_terms( root );
    match_symmetries( root );
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
