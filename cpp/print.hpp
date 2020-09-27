#include <iostream>
#include <set>
#include "syntax_tree.hpp"
using namespace std;

void print_evaluation( const auto & layer, Node * root ) {
    cout << "Topo evaluation has stopped due to lock or exhaustion. Nodes evaluated: " << layer.evaluated.size() << ":" << endl;
    cout << "    facts:" << endl;
    for ( const auto & e : layer.evaluated )
        cout << "        " << e << endl;
    cout << "    values:" << endl;
    for ( const auto & v : layer.values )
        cout << "        " << v.first << " : " << v.second << endl;
    
    set< Node * > needs_evaluation;
    const auto & on_ev = [&]( Node * ev ) {
        if ( ev->type( set{ TYPE::EXPRESSION, TYPE::TERM } ) && layer.evaluated.find( ev ) == layer.evaluated.end() )
            needs_evaluation.insert( ev );
    };
    pulse( root, on_ev );
    cout << "    " << needs_evaluation.size() << " need to be evaluated:" << endl;
    for ( const auto & e : needs_evaluation ) {
        cout << "        " << e << endl;
    }
}
