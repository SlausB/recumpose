#include <string>
#include <set>
#include <iostream>
#include <fstream>
#include <cstdlib>
using namespace std;

/** Bidirectional (undirected) graphs.*/
void plot_dot(
    const set< pair< string, string > > & nodes,
    const map< string, string > & labels = map< string, string >(),
    const string & file_name = string( "graph.dot" ),
    const string & graph_name = string( "recumpose" )
) {
    ofstream o;
    o.open( file_name );

    o << "graph " << graph_name << " {" << endl;
    for ( const auto & l : labels ) {
        o << "    " << l.first << " [label=\"" << l.second << "\"];" << endl;
    }
    for ( const auto & n : nodes ) {
        o << "    " << n.first << " -- " << n.second << ";" << endl;
    }
    o << "}" << endl;

    const string command = string( "neato -Tpng " ) + file_name + " -o " + graph_name + ".png";
    system( command.c_str() );
}
