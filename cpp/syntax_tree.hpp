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
    /** Starts from 1 as it is in text editors.*/
    int32_t line;
    /** Which characters this node spans within source. Starts from 1 as it is in text editors.*/
    int32_t char_start;
    int32_t char_end;
    string file;

    SourcePos(
        const auto & line,
        const auto & char_start,
        const auto & char_end,
        const string file = ""
    ): line(line), char_start(char_start), char_end(char_end), file(file)
    {}

    /** Calculate child SourcePos at specified displacement and length.*/
    SourcePos disp( const int32_t disp, const int32_t length ) {
        return SourcePos(
            line,
            char_start + disp,
            char_start + disp + length,
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
