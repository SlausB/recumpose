
#include "syntactic.hpp"
#include "semantic.hpp"

/** Compiled strictly typed non-deterministic programming language based on lambda-calculus, reactive programming, homotopy type theory and fractal growth to easily handle composition.*/

/**
The whole execution/data (remember code-data equivalence) model takes shape of bidirectional graph (actually probably the most abstract data structure you can't even reason about).
Simplest unit with which programs are written in this programming language: doesn't have any nature (properties) or "parts": basically just named dots.
Compositions of these resemble functions (functions can be built with these). Roll Safe: "Why give definition to recursive function if you can have a recursive definition of a function?". Recursion defines the topology of propagation graph on top of which entity fractals grow and thus only sinks are kept to constitute the further growth (the growth itself IS the computer program). Some of the growth might be performed on compile time, but not always (this technique is equivalent to 'dependent types').
These entities are like reactive outputs: only resulting value on single entity (entity pluralism might be achieved with another composition though) regards the intended meaning: all the fractaled values are dropped (but only after they're lazily evaluated).
struct Entity {};
*/

/** Should return source name's caption i.e. "falcon" from "../samples/falcon.rcl", etc..*/
string source_caption( const string & source_name ) {
    auto start_pos = source_name.find_last_of( '/' );
    if ( start_pos != string::npos )
        ++ start_pos;
    if ( start_pos >= source_name.size() )
        start_pos = string::npos;

    const auto last_pos = source_name.find_last_of( '.' );
    const auto len =
        last_pos == string::npos || start_pos == string::npos
        ?
        string::npos
        :
        last_pos - start_pos
    ;

    return source_name.substr( start_pos,  len );
}

auto process( const string & source_name )
{
    auto root = syntactic( source_name );

    const auto target_name = source_caption( source_name );
    cout << "Plotting to " << target_name << endl;
    plot( root, set{ TYPE::EXPRESSION, TYPE::TERM, TYPE::NONABELIAN }, target_name + "_semantics" );
    plot( root, set{ TYPE::EXPRESSION, TYPE::TERM, TYPE::ENTITY, TYPE::NONABELIAN }, target_name + "_expressions" );

    semantic( root );

    cout << "Done." << endl;
}

int main() {
    //process( "../samples/program_1.rcl" );
    //process( "../samples/falcon.rcl" );
    process( "../samples/square_equation.rcl" );

    //every composition needs to be reversible (so to define sqrt() function you'll have to define complex numbers and thus recompose (+),(-),(*), etc)

    //composition works on types as well

    //the resulting graph might get shrunk to the lowest possible size (although can get functionized if properly represented with lie-groups on data-types (which are algebraic since compositions are reversible)) and outputed as is or directly executed as regular functional program

    return 0;
}

/**
//redifinition: might be used with not just immune programming (where resulting logic represents the unity of compositions declarations), but also conditional logic ("'a' is 'b+c' instead of 'a' when 'c': 'c = a ∘= b + c'):
a ∘= b + c

//applicative definition: another logic added on top of 'a':
a ∘+ g + 1
*/

//when function (program/entity/...) is composed on top of itself, it means at least that it takes the same type as it returns (where type is the space and values are points in that space). But the result of such composition is the different type ofc. Implementation of computation on top of that composition (on top of expression of composition) spawns new instance of type (where it's called "value") and instance is the same on the left side of composition and the right side of composion (same besides having just similar value). Thus, the moment of such instantiation requires permorming the branching of the whole composition graph.

//one might consider this language as detangling of problem graph into physical machine memory (which programmer always has to manage)

//Composition might have a name. Such name might be used for another composition. Composition IS the Type. Immunal approach allows removing any Composition from program. Type is space to which values of that Type belong. Thus, immune state of program defines it's Shape.
//speculation: program execution, then, might be the transition from one Shape to another.

// array-like data structures are handles as values of Types, but in such a way that due to execution lazy-ness values are still hanging around (not getting shrunk to a single presentation for immunity purposes). Thus, execution model must be included in any final program recumpose produces (another exposure of data-code equivalence).
