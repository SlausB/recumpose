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
#include <algorithm>

using namespace std;

enum TYPE {
    SOURCE_FILE,
    LINE,

    TERM,
    NUMBER,

    OPERATOR,

    PARENTH_BLOCK,

    INPUTS,
    OUTPUTS,

    EQUALITY,
    //the sophisticated (recursive?) one, not just regular function composition; regular one refers to EXPRESSION:
    COMPOSITION,
    /** Something just named having a bunch of compositions in it. Adds it's compositions into program immunity and can be composed on top as well. Entities are Types in terms of type-theory. Operations on Entities are operations on Types such that type can be "removed" from the set of executing program.*/
    ENTITY,

    SEMANTIC,

    /** Abstract construct which references operator and other terms or expressions.*/
    EXPRESSION,

    IF,
    THEN,
    ELSE,

    /** Just to break the symmetry of certain Operators (EXPRESSIONs) which have notion of "left" and "right".*/
    NONABELIAN,
};
ostream & operator <<( ostream & os, const TYPE & o ) {
    switch ( o ) {
        case TYPE::SOURCE_FILE  : os << "SOURCE_FILE"  ; break;
        case TYPE::LINE         : os << "LINE"         ; break;
        case TYPE::TERM         : os << "TERM"         ; break;
        case TYPE::NUMBER       : os << "NUMBER"       ; break;
        case TYPE::OPERATOR     : os << "OPERATOR"     ; break;
        case TYPE::PARENTH_BLOCK: os << "PARENTH_BLOCK"; break;
        case TYPE::INPUTS       : os << "INPUTS"       ; break;
        case TYPE::OUTPUTS      : os << "OUTPUTS"      ; break;
        case TYPE::EQUALITY     : os << "EQUALITY"     ; break;
        case TYPE::COMPOSITION  : os << "COMPOSITION"  ; break;
        case TYPE::ENTITY       : os << "ENTITY"       ; break;
        case TYPE::SEMANTIC     : os << "SEMANTIC"     ; break;
        case TYPE::EXPRESSION   : os << "EXPRESSION"   ; break;
        case TYPE::IF           : os << "IF"           ; break;
        case TYPE::THEN         : os << "THEN"         ; break;
        case TYPE::ELSE         : os << "ELSE"         ; break;
        case TYPE::NONABELIAN   : os << "NONABELIAN"   ; break;
    }
    return os;
}

enum OPERAND {
    INFIX,
    LEFT,
    RIGHT,
    RIGHT_ALL,
};
ostream & operator <<( ostream & os, const OPERAND & o ) {
    switch ( o ) {
        case OPERAND::INFIX    : os << "INFIX"    ; break;
        case OPERAND::LEFT     : os << "LEFT"     ; break;
        case OPERAND::RIGHT    : os << "RIGHT"    ; break;
        case OPERAND::RIGHT_ALL: os << "RIGHT_ALL"; break;
    }
    return os;
}

enum NONABELIAN_TYPE {
    ABELIAN,
    NON_ABELIAN,
};

struct OperatorDesc {
    string text;
    OPERAND operand;
    NONABELIAN_TYPE nonabelian = NONABELIAN_TYPE::ABELIAN;

    OperatorDesc( const string & text, const OPERAND & operand, const NONABELIAN_TYPE & nonabelian = NONABELIAN_TYPE::ABELIAN ): text(text), operand(operand), nonabelian(nonabelian) {}
};

/** In semantics matching order.*/
const vector< OperatorDesc > OperatorsDesc = {
    //OperatorDesc( "!", RIGHT ),
    { "!"      , RIGHT },
    { ":"      , INFIX },

    { "+"      , INFIX },
    { "-"      , INFIX, NON_ABELIAN },
    { "*"      , INFIX },
    { "/"      , INFIX, NON_ABELIAN },

    { "not"    , RIGHT },
    { "<"      , INFIX, NON_ABELIAN },
    { "=="     , INFIX },
    { ">"      , INFIX, NON_ABELIAN },
    { "<="     , INFIX, NON_ABELIAN },
    { ">="     , INFIX, NON_ABELIAN },
    { "if"     , RIGHT },
    { "then"   , RIGHT },
    { "else"   , RIGHT },

    { "->"     , INFIX, NON_ABELIAN },
    { "<-"     , INFIX, NON_ABELIAN },

    { "include", RIGHT_ALL },

    { "inputs" , RIGHT_ALL },
    { "outputs", RIGHT_ALL },

    { "="      , INFIX },

    { "∘"      , INFIX, NON_ABELIAN },
    { "∘="     , INFIX, NON_ABELIAN },
    { "∘+"     , INFIX, NON_ABELIAN },
    { "@"      , INFIX, NON_ABELIAN },
    { "@="     , INFIX, NON_ABELIAN },
    { "@+"     , INFIX, NON_ABELIAN },

    { "("      , RIGHT },
    { ")"      , LEFT  },
    { ";"      , LEFT  },
};

const vector< string > Operators = []{
    vector< string > data;
    for ( const auto & desc : OperatorsDesc ) { // or with c++20 : ranges & use set's InputIt constructor
        data.push_back( desc.text );
    }
    return data;
}();
const map< string, OPERAND > Operands = []{
    map< string, OPERAND > data;
    for ( const auto & desc : OperatorsDesc ) {
        data[ desc.text ] = desc.operand;
    }
    return data;
}();
const map< string, NONABELIAN_TYPE > NonAbelians = []{
    map< string, NONABELIAN_TYPE > na;
    for ( const auto & desc : OperatorsDesc ) {
        na[ desc.text ] = desc.nonabelian;
    }
    return na;
}();

/** Operators sorted from long to short.*/
const vector< string > LongerOperators = []{
    vector< string > data = Operators;
    //TODO: it thinks of "∘=" as being 4 chars long:
    sort( data.begin(), data.end(), []( const string & l, const string & s ) { return l.size() > s.size(); } );
    return data;
}();

struct SourcePos {
    string file;
    /** 1-based numbered as it is in text editors.*/
    int32_t line;
    /** Which characters this node spans within source. 1-based numbered as it is in text editors.*/
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

    bool intersects( const SourcePos & o, const bool & check_file = true ) const {
        if ( check_file && file != o.file )
            return false;
        if ( line != o.line )
            return false;
        if ( char_start > o.char_end )
            return false;
        if ( char_end < o.char_start )
            return false;
        return true;
    }

    auto operator<=>(const SourcePos&) const = default;
};
ostream & operator <<( ostream & os, const SourcePos & s ) {
    os << "{\"" << s.file << "\":" << s.line << ':' << s.char_start << '-' << s.char_end << '}';
    return os;
}

struct Node {
    string content;
    set< TYPE > types;
    SourcePos source_pos;
    /** Which other nodes this one references.*/
    set< Node * > refs;
    /** Which other nodes reference this one.*/
    set< Node * > refd;

    Node(
        const auto & content,
        const auto & type,
        SourcePos source_pos
    ): content(content), types{type}, source_pos(source_pos)
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
    void unref( Node * target ) {
        refs.erase( target );
        target->refd.erase( this );
    }
    /** Returns first occurence of Node with specified TYPE which references this Node.*/
    Node * parent( const TYPE & type ) {
        for ( auto & parent : refd ) {
            if ( parent->type( type ) )
                return parent;
        }
        return nullptr;
    }
    Node * parent( const auto & types ) {
        for ( auto & parent : refd ) {
            for ( const auto & type : types )
                if ( parent->type( type ) )
                    return parent;
        }
        return nullptr;
    }
    /** Returns first occurense of Node with specified TYPE which current Node references.*/
    Node * child( const TYPE & type ) {
        for ( auto & child : refs ) {
            if ( child->type( type ) )
                return child;
        }
        return nullptr;
    }

    /** Returns true if has specified type.*/
    bool type( const TYPE & type ) {
        return types.find( type ) != types.end();
    }
    bool type( const auto & types ) {
        for ( const auto & t : types ) {
            if ( type( t ) )
                return true;
        }
        return false;
    }
};
ostream & operator <<( ostream & os, const Node * node ) {
    os << "{ ";
    bool printed_type = false;
    for ( const auto & type : node->types ) {
        if ( printed_type )
            cout << "|";
        printed_type = true;
        os << type;
    }
    os << " \"" << node->content << "\" at " << node->source_pos << " }";
    return os;
}
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
    int32_t r = 0;
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
@param types_filter set of TYPEs over which on traverse should follow. If empty traverse follows over any TYPEs passing those specified in types_wall.
@param types_wall set of TYPEs over which traverse should NOT follow. If empty traverse follows over any TYPEs specified in types_filter.
*/
template<
    bool Refs = true,
    bool Refd = true,
    typename TypesFilter = bool,
    typename TypesWall = bool,
    typename Func
>
void pulse(
    Node * root,
    Func const & on_node,
    TypesFilter const & types_filter = false,
    TypesWall const & types_wall = false
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
                    if ( ! next->type( types_filter ) )
                        continue;
                }

                //skip if pass specified and does have current TYPE:
                if constexpr ( ! is_same_v< TypesWall, bool > ) {
                    if ( next->type( types_wall ) )
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
        if ( node->type( type ) ) {
            result = node;
            return false;
        }
        return true;
    };
    pulse( center, on_node );
    return result;
}

Node * find_types( auto & in, const auto & types ) {
    for ( auto r : in ) {
        if ( r->type( types ) )
            return r;
    }
    return nullptr;
}

void print_lines( Node * root ) {
    cout << "Source split into lines:" << endl;
    const auto & print = []( Node * node ) {
        if ( ! node->type( TYPE::LINE ) )
            return true;
        cout << "    " << node->source_pos.line << ": chars " << node->source_pos.char_start << "-" << node->source_pos.char_end << ": " << node->content << endl;
        return true;
    };
    pulse( root, print );
}




