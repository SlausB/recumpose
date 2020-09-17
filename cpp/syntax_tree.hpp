#pragma once

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
#include <ranges>

using namespace std;

enum TYPE {
    SOURCE_FILE,
    LINE,
    CHAR,

    TERM,

    OPERATOR,

    PARENTH_BLOCK,

    INPUTS,
    OUTPUTS,

    EQUALITY,
    //the sophisticated (recursive?) one, not just regular function composition; regular one refers to EXPRESSION:
    COMPOSITION,
    /** Something just named having a bunch of compositions in it. Adds it's compositions into program immunity and can be composed on top as well.*/
    ENTITY,

    SEMANTIC,

    /** Abstract construct which references operator and other terms or expressions.*/
    EXPRESSION,
};

enum OPERAND {
    INFIX,
    LEFT,
    RIGHT,
};

const vector< pair< string, OPERAND > > OperatorsDesc = {
    { "!"      , RIGHT },
    { ":"      , INFIX },

    { "+"      , INFIX },
    { "-"      , INFIX },
    { "*"      , INFIX },
    { "/"      , INFIX },

    { "->"     , INFIX },
    { "<-"     , INFIX },

    { "if"     , RIGHT },
    { "then"   , RIGHT },
    { "else"   , RIGHT },
    { "<"      , INFIX },
    { "=="     , INFIX },
    { ">"      , INFIX },
    { "<="     , INFIX },
    { ">="     , INFIX },
    { "not"    , RIGHT },

    { "include", RIGHT },

    { "inputs" , RIGHT },
    { "outputs", RIGHT },

    { "="      , INFIX },

    { "∘"      , INFIX },
    { "∘="     , INFIX },
    { "∘+"     , INFIX },
    { "@"      , INFIX },
    { "@="     , INFIX },
    { "@+"     , INFIX },

    { "("      , RIGHT },
    { ")"      , LEFT  },
    { ";"      , LEFT  },
};

/** Matching is performed in order.*/
//const string Operators[] = statically_extract_pair_first_elements( ) {};
const vector< string > Operators = []{
    vector< string > data;
    for ( const auto & pair : OperatorsDesc ) { // or with c++20 : ranges & use set's InputIt constructor
        data.push_back( pair.first );
    }
    return data;
}();

const map< string, OPERAND > Operands = []{
    map< string, OPERAND > data;
    for ( const auto & pair : OperatorsDesc ) {
        data[ pair.first ] = pair.second;
    }
    return data;
}();

struct SourcePos {
    string file;
    /** Starts from 1 as it is in text editors.*/
    int32_t line;
    /** Which characters this node spans within source. Starts from 1 as it is in text editors.*/
    int32_t char_start;
    int32_t char_end;

    SourcePos(
        const auto & line,
        const auto & char_start,
        const auto & char_end,
        const string file = ""
    ): file(file), line(line), char_start(char_start), char_end(char_end)
    {}

    /** Calculate child SourcePos at specified displacement and length.*/
    SourcePos disp( const int32_t disp, const int32_t length ) {
        return SourcePos(
            line,
            char_start + disp,
            char_start + disp + length - 1,
            file
        );
    }

    auto operator<=>(const SourcePos&) const = default;
};
ostream & operator <<( ostream & os, const SourcePos & s ) {
    os << '{' << s.line << ':' << s.char_start << '-' << s.char_end << '}';
    return os;
}

struct Node {
    string content;
    TYPE type;
    SourcePos source_pos;
    /** Which other nodes this one references.*/
    set< Node * > refs;
    /** Which other nodes reference this one.*/
    set< Node * > refd;

    Node(
        const auto & content,
        const auto & type,
        SourcePos source_pos
    ): content(content), type(type), source_pos(source_pos)
    {}
    ~Node() {
        for ( auto & ref : refs )
            ref->refd.erase( this );
        for ( auto & red : refd )
            red->refs.erase( this );
    }

    /** Make this Node reference other specified Node.*/
    void ref( Node * target ) {
        refs.insert( target );
        target->refd.insert( this );
    }
    /** Returns first occurence of Node with specified TYPE which references this Node.*/
    Node * parent( const TYPE & type ) {
        for ( auto & parent : refd ) {
            if ( parent->type == type )
                return parent;
        }
        return nullptr;
    }
    Node * parent( const auto & types ) {
        for ( auto & parent : refd ) {
            for ( const auto & type : types )
                if ( parent->type == type )
                    return parent;
        }
        return nullptr;
    }
    /** Returns first occurense of Node with specified TYPE which current Node references.*/
    Node * child( const TYPE & type ) {
        for ( auto & child : refs ) {
            if ( child->type == type )
                return child;
        }
        return nullptr;
    }
};
/** Used to reason about nodes relative positioning in line.*/
struct NodeHandler {
    Node * node;
    NodeHandler( Node * node ): node(node) {}
    bool operator < ( const NodeHandler & n ) const {
        return node->source_pos < n.node->source_pos;
    }
};
/** Sort Nodes regarding their presence in source code.*/
void syntactic_position_sort( auto & nodes ) {
    nodes.sort(
        []( Node * left, Node * right ) { return left->source_pos < right->source_pos; }
    );
}
int32_t indentation( Node * line ) {
    int32_t r;
    for ( auto & ch : line->content ) {
        if ( isspace( ch ) )
            ++ r;
        else
            break;
    }
    return r;
}
/** Returns true if first TYPE::LINE has more indentation than second.*/
bool increased_indentation( Node * increased, Node * then ) {
    return indentation( increased ) > indentation( then );
}
bool equal_indentation( Node * one_line, Node * another_line ) {
    return indentation( one_line ) == indentation( another_line );
}

/** Breadth first iteration over AST.
@param types_filter set of TYPEs over which on traverse should follow. If empty traverse follows over any TYPEs passing those specified in types_pass.
@param types_pass set of TYPEs over which traverse shouuld NOT follow. If empty traverse follows over any TYPEs specified in types_filter.
*/
template<
    bool Refs = true,
    bool Refd = true,
    typename TypesFilter = bool,
    typename TypesPass = bool,
    typename Func
>
void pulse(
    Node * root,
    Func const & on_node,
    TypesFilter const & types_filter = false,
    TypesPass const & types_pass = false
) {
    set< Node * > visited;
    visited.insert( root );

    set< Node * > to_visit;
    to_visit.insert( root );

    while ( ! to_visit.empty() ) {
        auto v_it = to_visit.begin();
        to_visit.erase( v_it );
        auto node = * v_it;

        if constexpr ( is_same_v< invoke_result_t< decltype( on_node ), Node* >, bool > ) {
            if ( ! on_node( node ) )
                return;
        }
        else
            on_node( node );
        
        const auto & visit = [&]( set< Node * > & array ) {
            //visit adjacents later on:
            for ( auto next : array ) {
                //skip if filter specified and doesn't have current TYPE:
                if constexpr ( ! is_same_v< TypesFilter, bool > ) {
                    if ( types_filter.find( next->type ) == types_filter.end() )
                        continue;
                }

                //skip if pass specified and does have current TYPE:
                if constexpr ( ! is_same_v< TypesPass, bool > ) {
                    if ( types_pass.find( next->type ) != types_pass.end() )
                        continue;
                }

                //... only if wasn't visited yet:
                if ( visited.insert( next ).second )
                    to_visit.insert( next );
            }
        };
        if constexpr ( Refs )
            visit( node->refs );
        if constexpr ( Refd )
            visit( node->refd );
        static_assert( Refs || Refd, "pulse(): should traverse at least some direction" );
    }
}
/** Returns first occurence of Node with specified TYPE around center Node or nullptr.*/
Node * closest( Node * center, const TYPE type ) {
    Node * result = nullptr;
    const auto & on_node = [&]( Node * node ) {
        if ( node->type == type ) {
            result = node;
            return false;
        }
        return true;
    };
    pulse( center, on_node );
    return result;
}

Node * find_type( auto & in, const TYPE type ) {
    for ( auto r : in ) {
        if ( r->type == type )
            return r;
    }
    return nullptr;
}

void print_lines( Node * root ) {
    cout << "Source split into lines:" << endl;
    const auto & print = []( Node * node ) {
        if ( node->type != TYPE::LINE )
            return true;
        cout << "    " << node->source_pos.line << ": chars " << node->source_pos.char_start << "-" << node->source_pos.char_end << ": " << node->content << endl;
        return true;
    };
    pulse( root, print );
}

/** Returns true if character participates in Node of specified TYPE.*/
bool check_char(
    Node * root,
    const TYPE type,
    const SourcePos & source_pos
) {
    bool does = false;
    const auto & on_char = [&]( Node * char_node ) {
        if ( char_node->type != TYPE::CHAR )
            return true;
        if ( char_node->source_pos == source_pos ) {
            for ( const auto & red : char_node->refd ) {
                if ( red->type == type ) {
                    does = true;
                    return false;
                }
            }
            return false;
        }
        return true;
    };
    pulse( root, on_char );
    return does;
}




