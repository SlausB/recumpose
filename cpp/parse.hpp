
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
        file_name,
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
        if ( ! line_node->type( TYPE::LINE ) )
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
        if ( ! file_node->type( TYPE::SOURCE_FILE ) )
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

/** Returns true if specified string contains of alphabetic characters only.*/
bool is_alpha_string( const string & s ) {
    for ( const auto ch : s ) {
        if ( ! isalpha( ch ) )
            return false;
    }
    return true;
}

/** Returns true if specified SourcePos intersects with any existing element on specified LINE.*/
bool intersects_any_on_line( const Node * line, const SourcePos & s ) {
    for ( auto line_el : line->refs ) {
        if ( line_el->type( set{ TYPE::SOURCE_FILE, TYPE::LINE } ) )
            continue;
        if ( line_el->source_pos.intersects( s, false ) )
            return true;
    }
    return false;
}

bool match_operator(
    Node * line_node,
    const string & op,
    size_t & caret
) {
    const auto r = line_node->content.find( op, caret );

    if ( r == string::npos )
        return false;
    
    if ( intersects_any_on_line( line_node, line_node->source_pos.disp( r, op.size() ) ) )
        return false;

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
    caret = r + op.size();
    return true;
}

void match_operators( Node * root ) {
    const auto & on_line = []( Node * line_node ){
        if ( ! line_node->type( TYPE::LINE ) )
            return true;
        
        for ( const auto & op : LongerOperators ) {
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
        if ( ! line_node->type( TYPE::LINE ) )
            return true;
        
        int32_t seq_start = 0;
        for ( size_t caret = 0; caret <= line_node->content.size(); ++ caret ) {
            if (
                caret >= line_node->content.size()
                ||
                ! isalnum( line_node->content.at( caret ) )
                ||
                //check that not yet an operator:
                intersects_any_on_line( line_node, line_node->source_pos.disp( caret, 1 ) )
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
        if ( ! file_node->type( TYPE::SOURCE_FILE ) )
            return;
        
        //expression that is currently being parsed:
        Node * caret = nullptr;
        
        auto line_node = file_node;
        while ( ( line_node = line_node->child( TYPE::LINE ) ) ) {
            list< Node * > terms_and_ops;

            for ( auto & to_ref : line_node->refs ) {
                if ( to_ref->type( set{ TYPE::TERM, TYPE::OPERATOR } ) ) {
                    terms_and_ops.push_back( to_ref );
                }
            }

            syntactic_position_sort( terms_and_ops );
            for ( auto & to : terms_and_ops ) {
                if ( to->type( TYPE::TERM ) )
                    cache[ file_node ].terms.push_back( to );
                if ( to->type( TYPE::OPERATOR ) )
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

/** @param or_stop set of contents to stop on if specified and reached.*/
template< typename Stop = bool >
Node * ultimate_parent_expression( Node * source, const Stop & or_stop = false ) {
    const auto & on_parent = [&]( Node * parent ) {
        source = parent;
        if constexpr ( ! is_same_v< Stop, bool > ) {
            for ( const auto & content : or_stop ) {
                if ( source->content == content )
                    return false;
            }
        }
        return true;
    };
    pulse< false >( source, on_parent, set{ TYPE::EXPRESSION } );
    return source;
}

/** Obtain term at specified Node's relation up to it's most composed EXPRESSION.*/
template< typename Stop = bool >
Node * relative_term_up_to_expression( auto & from, const Stop & or_stop = false ) {
    auto rel = find_types( from, vector{ TYPE::OPERATOR, TYPE::TERM } );
    if ( rel == nullptr )
        return nullptr;
    return ultimate_parent_expression( rel, or_stop );
}

auto check_rel_presence( const Node * from, const Node * term, const string & orient ) {
    if ( term == nullptr ) {
        cout << "ERROR: no term at " << orient << " from operator " << from->content << " at " << from->source_pos << endl;
        throw runtime_error( "semantics error" );
    }
}

Node * consume_left( Node * op, Node *& expr ) {
    auto left = relative_term_up_to_expression( op->refd );
    check_rel_presence( op, left, "left" );

    if ( expr == nullptr )
        expr = new Node(
            op->content + " expression",
            TYPE::EXPRESSION,
            op->source_pos
        );
    expr->ref( op );
    expr->ref( left );

    return left;
}
Node * consume_right( Node * op, Node *& expr ) {
    auto right = relative_term_up_to_expression( op->refs );
    check_rel_presence( op, right, "right" );

    if ( expr == nullptr )
        expr = new Node(
            op->content + " expression",
            TYPE::EXPRESSION,
            op->source_pos
        );
    expr->ref( op );
    expr->ref( right );

    return right;
}
Node * consume_infix( Node * op ) {
    Node * expr = nullptr;
    auto left = consume_left( op, expr );
    consume_right( op, expr );
    
    if ( NonAbelians.at( op->content ) == NONABELIAN_TYPE::NON_ABELIAN ) {
        auto nonabelian = new Node(
            op->content + " nonabelian",
            TYPE::NONABELIAN,
            op->source_pos
        );
        expr->ref( nonabelian );
        nonabelian->ref( left );
    }

    return expr;
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
            Node * expr = nullptr;

            switch ( Operands.at( op->content ) ) {
                case OPERAND::INFIX:
                    expr = consume_infix( op );
                    break;
                case OPERAND::LEFT:
                    consume_left( op, expr );
                    break;
                case OPERAND::RIGHT:
                    consume_right( op, expr );
                    break;
                //will be matched later on:
                case OPERAND::RIGHT_ALL:
                    continue;
            }

            if ( expr == nullptr ) {
                const string error = string( "ERROR: operator " ) + op->content + " was NOT matched.";
                cout << error << endl;
                throw runtime_error( error );
            }
            else {
                cout << "Operator " << op << " successfully matched into EXPRESSION " << expr << ":" << endl;
                for ( const auto & ref : expr->refs )
                    cout << "    " << ref << endl;
            }
        }
    }
}

auto bottom_semantics( Node * node ) {
    list< Node * > bottom;
    const auto & on_child = [&]( Node * child ) {
        for ( auto deep : child->refs ) {
            if ( deep->type( set{ TYPE::TERM, TYPE::OPERATOR } ) )
                bottom.push_back( deep );
        }
    };
    pulse< true, false >( node, on_child, set{ TYPE::EXPRESSION } );
    syntactic_position_sort( bottom );
    return bottom;
}

void reglue_parent_expr( Node * node, Node * to ) {
    auto parent = node->parent( TYPE::EXPRESSION );
    if ( parent != nullptr ) {
        parent->unref( node );
        parent->ref( to );
    }
}

void merge_ifs( Node * root ) {
    const auto & on_node = []( Node * node ) {
        if ( ! node->type( TYPE::EXPRESSION ) )
            return;
        
        //if not top-level expression:
        if ( find_types( node->refd, TYPE::EXPRESSION ) )
            return;
        
        if ( node->content == "then expression" ) {
            auto left = relative_term_up_to_expression( bottom_semantics( node ).front()->refd, vector{ "if expression" } );
            if ( left == nullptr || left->content != "if expression" ) {
                ostringstream str;
                str << "ERROR: no if expression at left of " << node;
                cout << str.str() << endl;

                cout << "    left: " << left << endl;
                const auto bottom = bottom_semantics( node );
                cout << "    it has bottom semantics (syntactically ordered):" << endl;
                for ( const auto & b : bottom )
                    cout << "        " << b << endl;
                
                cout << "    referenced by:" << endl;
                for ( const auto & pr : bottom_semantics( node ).front()->refd )
                    cout << "        " << pr << endl;
                
                cout << "    refs:" << endl;
                for ( const auto & ref : node->refs ) {
                    cout << "        " << ref << endl;

                    cout << "            red:" << endl;
                    for ( const auto & red : ref->refd ) {
                        cout << "                " << red << endl;
                    }
                }

                throw runtime_error( str.str() );
            }

            auto if_then = new Node(
                "if-then expression",
                TYPE::EXPRESSION,
                left->source_pos
            );

            //reglue "if" expression to this new one:
            reglue_parent_expr( left, if_then );

            if_then->ref( left );
            if_then->ref( node );
            cout << "Matched then with else: " << if_then << endl;
        }
        else if ( node->content == "else expression" ) {
            auto left = relative_term_up_to_expression( bottom_semantics( node ).front()->refd, vector{ "if-then expression" } );
            if ( left == nullptr || left->content != "if-then expression" ) {
                ostringstream str;
                str << "ERROR: no if-then expression at left of " << node;
                cout << str.str() << endl;
                throw runtime_error( str.str() );
            }

            auto if_then_else = new Node(
                "if-then-else expression",
                TYPE::EXPRESSION,
                left->source_pos
            );

            //reglue "if-then" expression to this new one:
            reglue_parent_expr( left, if_then_else );

            if_then_else->ref( left );
            if_then_else->ref( node );
            cout << "Matched else with if-then: " << if_then_else << endl;
        }
    };
    pulse( root, on_node );
}

Node * find_line_from_expression( Node * expr ) {
    Node * result = nullptr;
    const auto & on_node = [&]( Node * node ) {
        if ( node->type( set{ TYPE::TERM, TYPE::OPERATOR } ) ) {
            result = find_types( node->refd, TYPE::LINE );
            return false;
        }
        return true;
    };
    pulse< true, false >( expr, on_node );
    return result;
}

Node * consume_right_until_indentation( Node * from, auto & syntactic_it, const string & name, const auto & until ) {
    Node * line = find_line_from_expression( from );
    Node * rolling = nullptr;
    while ( syntactic_it != until ) {
        auto right = * syntactic_it;

        auto right_line = find_line_from_expression( right );
        if ( right_line != line && ! increased_indentation( right_line, line ) ) {
            cout << "        different lines " << line << " AND " << right_line << " and NOT increased indentation" << endl;
            break;
        }
        
        if ( rolling == nullptr ) {
            rolling = new Node(
                from->content + " " + name,
                TYPE::EXPRESSION,
                from->source_pos
            );
            rolling->ref( from );
            cout << "        spawned EXPRESSION for left " << from << endl;
        }
        cout << "        spawned RIGHT " << right << endl;
        rolling->ref( right );
        ++ syntactic_it;
    }
    return rolling;
}

void match_right_all( Node * file ) {
    set< Node * > top_level_set;
    const auto & on_node = [&]( Node * node ) {
        if ( node->type( set{ TYPE::LINE, TYPE::SOURCE_FILE } ) )
            return;
        //if NOT top level expression:
        if ( find_types( node->refd, vector{ TYPE::EXPRESSION, TYPE::ENTITY } ) != nullptr ) {
            cout << "    skipping node " << node << " because it's NOT top-level since it's referenced by " << find_types( node->refd, vector{ TYPE::EXPRESSION, TYPE::ENTITY } ) << endl;
            return;
        }
        top_level_set.insert( node );
    };
    pulse( file, on_node, false, set{ TYPE::SOURCE_FILE } );

    list< Node * > top_level_list;
    for ( auto & n : top_level_set )
        top_level_list.push_back( n );
    syntactic_position_sort( top_level_list );

    cout << "Top level semantic nodes of file " << file->content << " in syntactic order:" << endl;
    auto tl_it = top_level_list.begin();
    while ( tl_it != top_level_list.end() ) {
        auto tl = * tl_it;
        cout << "    " << tl << endl;
        ++ tl_it;

        //RIGHT_ALL operand:
        if ( tl->type( TYPE::OPERATOR ) ) {
            auto op = consume_right_until_indentation( tl, tl_it, "operator", top_level_list.end() );
            if ( tl->content == "inputs" )
                op->types.insert( TYPE::INPUTS );
            else if ( tl->content == "outputs" )
                op->types.insert( TYPE::OUTPUTS );
            else
                cout << "ERROR: undefined RIGHT_ALL operator." << endl;
            continue;
        }

        //entity:
        if ( tl->type( TYPE::TERM ) ) {
            auto entity = consume_right_until_indentation( tl, tl_it, "entity", top_level_list.end() );
            entity->types.insert( TYPE::ENTITY );
            //TODO: maybe recursively? To handle potential entities defined as part of bigger entities (does it make any sense though?) ...
            continue;
        }
    }
}
void match_right_all_files( Node * root ) {
    const auto & on_file = []( Node * file_node ) {
        if ( file_node->type( TYPE::SOURCE_FILE ) )
            match_right_all( file_node );
    };
    pulse( root, on_file );
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

    {
        map< Node *, FileCache > cache;
        chain_terms_and_operators( root, cache );
        match_semantics( cache );
    }

    merge_ifs( root );
    match_right_all_files( root );

    return root;
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

/** All yet syntactically the same TERMs should merge into a single TERM (all EXPRESSIONs references need to repoint) and those dropped are removed.*/
void merge_occurences( Node * root ) {
    set< Node * > remove;
    map< string, Node * > terms;

    const auto & on_term = [&]( Node * term ) {
        if ( ! term->type( TYPE::TERM ) )
            return;
        
        auto existing = terms[ term->content ];
        if ( existing == nullptr ) {
            terms[ term->content ] = term;
            return;
        }

        //move all expressions that reference one of these occurences into another one:
        cout << "Merge occurence " << term << " into " << existing << endl;
        for ( auto expr : term->refd ) {
            if ( expr->type( set{ TYPE::EXPRESSION, TYPE::NONABELIAN } ) ) {
                cout << "    " << expr << " -> " << existing << endl;
                expr->ref( existing );
            }
        }
        remove.insert( term );
    };
    pulse( root, on_term );

    for ( auto & r : remove )
        delete r;
}

bool try_use(
    Node * node,
    const map< Node *, int64_t > & evaluated,
    int64_t & result
) {
    if ( node->type( TYPE::TERM ) ) {
        if ( node->type( TYPE::NUMBER ) ) {
            result = stol( node->content );
            return true;
        }
        throw runtime_error( "ERROR: undefined term" );
    }
    //other expression:
    auto ev = evaluated.find( node );
    if ( ev == evaluated.end() )
        return false;
}

/** Extract left and right nodes of specified operator.*/
bool extract( Node * op, Node *& left, Node *& right ) {
    const auto Operand = Operands.at( op->content );
    left = nullptr;
    right = nullptr;
    switch ( Operand ) {
        case OPERAND::INFIX:
            for ( auto & ref : op->refs ) {
                if ( ref->type( set{ TYPE::EXPRESSION, TYPE::TERM } ) )
                    left = ref;
                else if ( ref->type( TYPE::NONABELIAN ) )
                    right = * ref->refs.begin();
            }
            if ( left == nullptr || right == nullptr ) {
                stringstream s;
                s << "ERROR: couldn't resolve left and right operands of operator " << op;
                cout << s.str() << endl;
                throw runtime_error( s.str() );
            }
            break;
        
        case OPERAND::LEFT:
        case OPERAND::RIGHT:
            left = find_types( op->refs, set{ TYPE::EXPRESSION, TYPE::TERM } );
            break;
        case OPERAND::RIGHT_ALL:
            //TODO:
            break;
        default:
            stringstream s;
            s << "ERROR: unresolved operator's operand type " << op;
            cout << s.str() << endl;
            throw runtime_error( s.str() );
            break;
    }
}

bool try_evaluate(
    Node * expr,
    const map< Node *, int64_t > & evaluated,
    int64_t & value
) {
    auto op = find_types( expr->refs, TYPE::OPERATOR );
    if ( op == nullptr )
        throw runtime_error( "ERROR: Expected operator" );
    
    Node * left = nullptr;
    Node * right = nullptr;
    extract( op, left, right );

    auto left_it = evaluated.find( left );
    if ( left != nullptr && left_it == evaluated.end() )
        return false;
    auto right_it = evaluated.find( right );
    if ( right != nullptr && right_it == evaluated.end() )
        return false;
    
    if ( op->content == "/" ) {
        value = left_it->second / right_it->second;
    }
    else if ( op->content == "@+" ) {
        value = left_it->second + right_it->second;
    }
    else if ( op->content == "=" ) {
        //TODO:
    }
    else {
        throw runtime_error( string( "ERROR: undefined yet operator: " ) + op->content );
    }

    return true;
}

void try_evaluate_all( Node * root ) {
    //TODO: debug mode ofc, until evaluation is sufficiently abstract:
    map< Node *, int64_t > evaluated;

    //everything pulses once:
    bool moved = true;
    while ( moved ) {
        moved = false;
        const auto & on_expr = [&]( Node * expr ) {
            if ( ! expr->type( TYPE::EXPRESSION ) )
                return;

            //if every TERM this EXPRESSION depends on can be evaluated right now:
            bool free = true;
            for ( auto & ref : expr->refs ) {
                if ( ! ref->type( set{ TYPE::EXPRESSION, TYPE::TERM } ) )
                    continue;
                if ( evaluated.find( ref ) == evaluated.end() ) {
                    free = false;
                    break;
                }
            }
            if ( free ) {
                moved = true;
                int64_t value = 0;
                const auto was_evaluated = try_evaluate( expr, evaluated, value );
                if ( was_evaluated ) {
                    evaluated[ expr ] = value;
                }
            }
        };
        pulse( root, on_expr );
    }
    //every node should be evaluated by now because we're topologically locked (knotted?) ...
}

void intersect( Node * root ) {
    map< Node *, list< set< Node * > > > state;
}

auto parse( const string & file_name )
{
    print_file( file_name );
    auto root = parse_source( file_name );
    if ( root == nullptr )
        return;

    plot( root, set{ TYPE::EXPRESSION, TYPE::TERM, TYPE::NONABELIAN }, "semantics" );
    
    merge_occurences( root );

    plot( root, set{ TYPE::EXPRESSION, TYPE::TERM, TYPE::ENTITY, TYPE::NONABELIAN }, "expressions" );

    cout << "Done." << endl;
}
