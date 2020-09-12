
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <list>
#include <queue>
#include <set>

using namespace std;

auto source = R"V0G0N(
k = 100
x = 2
y = 3
b = 4
)V0G0N";

enum TYPE {
    LINE,

    TERM,

    OPERATOR,

    PARENTH_BLOCK,

    EQUALITY,
    COMPOSITION,
};

const auto Operators = {
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

    "(" ,
    ")" ,

    "if",
    "then",
    "else",
    "<",
    "==",
    ">",
    "<=",
    ">=",

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
        const string file = "",
    ): line(line), char_start(char_start), char_end(char_end), file(file)
    {}
};

struct Node {
    string content;

    /** References are always bidirectional because one might use SourcePos to reason about ordering.*/
    set< Node * > refs;

    TYPE type;

    SourcePos source_pos;

    Node(
        const auto & content,
        const auto & type,
        SourcePos source_pos
    ): content(content), type(type), source_pos(source_pos)
    {}

    void ref( Node * target ) {
        refs.insert( target );
        target->refs.insert( this );
    }
};

void depth_first( Node * root, const auto & on_node ) {
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

void print_lines( Node * root ) {
    cout << "Source split into lines:" << endl;
    const auto & print = []( Node * node ) {
        cout << "    " << node->source_pos.line << ": chars " << node->source_pos.char_start << "-" << node->source_pos.char_end << ": " << node->content << endl;
        return true;
    };
    depth_first( root, print );
}

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

void match_parenth( Node * root ) {
    //TODO:
}

void match_operators( Node * root ) {

}

auto parse()
{
    std::string input = source;
    if ( input.size() <= 0 ) {
        cout << "Empty program source." << endl;
        return;
    }
    
    cout << "Parsing the source:" << endl << source << endl;
    cout << "=======================================================" << endl;
    auto root = match_lines( input );
    print_lines( root );
    cout << "=======================================================" << endl;

    match_parenth( root );


    cout << "Done." << endl;
}
