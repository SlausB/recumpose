
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <list>
#include <queue>
#include <set>
#include <cctype>

using namespace std;

auto source = R"V0G0N(
k = 100 = l * 3
x = 2
y = 3
b = 4
)V0G0N";

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

void match_terms( Node * root ) {
    const auto & on_line = [&]( Node * line_node ) {
        if ( line_node->type != TYPE::LINE )
            return true;
        
        int32_t seq_start = 0;
        for ( size_t caret = 0; caret < line_node->content.size(); ++ caret ) {
            const auto ch = line_node->content.at( caret );
            if (
                ! isalnum( ch )
                ||
                //check if not yet an operator:
                check_char(
                    root,
                    TYPE::OPERATOR,
                    line_node->source_pos.disp( caret, 1 )
                )
                ||
                caret == line_node->content.size() - 1
            ) {
                const auto length = caret - seq_start;
                if ( length > 0 ) {
                    auto term_node = new Node(
                        line_node->content.substr( seq_start, length ),
                        TYPE::TERM,
                        line_node->source_pos.disp( seq_start, length )
                    );
                    term_node->ref( line_node );
                    cout << "Term " << term_node->content << " spawned at " << term_node->source_pos << endl;
                }
                seq_start = caret + 1;
            }
        }
        return true;
    };
    pulse( root, on_line );
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

    match_operators( root );
    fix_literal_ops( root );
    match_terms( root );

    cout << "Done." << endl;
}
