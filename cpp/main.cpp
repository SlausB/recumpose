
#include <list>
#include <iostream>
#include "parse.hpp"

using namespace std;

/** Compiled strictly typed non-deterministic programming language based on lambda-calculus, reactive programming, type theory and fractal growth to easily handle composition.*/

/** The whole execution/data (remember code-data equivalence) model takes shape of bidirectional graph (actually probably the most abstract data structure you can't even reason about).*/
struct Graph {

};

/** Simplest unit programs are written with in this programming language: don't have any nature or "parts": basically just named dots.
Compositions of these are like functions, but not (functions can be built with these). Roll Safe: "Why give definition to recursive function if you can have a recursive definition of function?". Recursion defines the topology of propagation graph on top of which entity fractals grow and thus only sinks are kept to constitute the further growth (the growth itself IS the computer program). Some of the growth might be performed on compile time, but not always (this technique is equivalent to 'dependent types').
These entities are like reactive outputs: only resulting value on single entity (entity pluralism might be achieved with another composition though) regards the intended meaning: all the fractaled values are dropped (but only after they're lazily evaluated).
*/
struct Entity {

};

int main() {
    parse();
    //parse_ast();

    //pulse_ast_with_numbered_waves_by_composing_layered_waves_on_entities();

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

//when function (program/endity/...) is composed on top of itself, it means at least that it takes the same type as it returns (where type is the space and values are points in that space). But the result of such composition is the different type ofc. Implementation of computation on top of that composition (on top of expression of composition) spawns new instance of type (where it's called "value") and instance is the same on the left side of composition and the right side of composion (same besides having just similar value). Thus, the moment of such instantiation requires permorming the branching of the whole composition graph.
