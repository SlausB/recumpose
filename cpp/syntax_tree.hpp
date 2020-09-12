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

using namespace std;

enum TYPE {
    LINE,
    CHAR,

    TERM,

    OPERATOR,

    PARENTH_BLOCK,

    EQUALITY,
    COMPOSITION,
};

/** Matching is performed in order.*/
const string Operators[] {
    "(" ,
    ")" ,

    ":" ,
    ";" ,

    "=" ,

    "->",
    "<-",

    "+" ,
    "-" ,
    "*" ,
    "/" ,

    "∘" ,
    "∘=",
    "∘+",

    "if",
    "then",
    "else",
    "<",
    "==",
    ">",
    "<=",
    ">=",

    "inputs",
    "outputs",
};

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
    /** References are always bidirectional because one might use SourcePos to reason about ordering.*/
    set< Node * > refs;

    Node(
        const auto & content,
        const auto & type,
        SourcePos source_pos
    ): content(content), type(type), source_pos(source_pos)
    {}
    ~Node() {
        for ( auto & ref : refs )
            ref->refs.erase( this );
    }

    void ref( Node * target ) {
        refs.insert( target );
        target->refs.insert( this );
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

/** Breadth first iteration over AST.*/
void pulse( Node * root, const auto & on_node ) {
    set< Node * > visited;
    visited.insert( root );

    set< Node * > to_visit;
    to_visit.insert( root );

    while ( ! to_visit.empty() ) {
        auto v_it = to_visit.begin();
        to_visit.erase( v_it );
        auto node = * v_it;

        if ( ! on_node( node ) )
            return;
        
        //visit it's children later on:
        for ( auto next : node->refs ) {
            //... only if wasn't visited already:
            if ( visited.insert( next ).second )
                to_visit.insert( next );
        }
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
            for ( const auto & ref : char_node->refs ) {
                if ( ref->type == type ) {
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




