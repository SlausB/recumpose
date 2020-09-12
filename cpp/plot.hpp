#pragma once

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

auto plot( Node * root ) {
    set< pair< string, string > > nodes;
    map< string, string > labels;

    const auto & should_plot = []( Node * node ) {
        switch ( node->type ) {
            case TYPE::LINE:
            case TYPE::TERM:
            case TYPE::OPERATOR:
                return true;
            default:
                return false;
        }
        return false;
    };
    const auto & name = [&]( Node * node ) {
        return to_string( (uint64_t) (void const *) node );
    };
    const auto & label = [&]( Node * node ) {
        switch ( node->type ) {
            case TYPE::LINE:
                return string( "line " ) + to_string( node->source_pos.line );
            default:
                return node->content;
        }
    };

    const auto & on_node = [&]( Node * node ) {
        if ( ! should_plot( node ) )
            return true;
        for ( const auto & ref : node->refs ) {
            if ( should_plot( ref ) ) {
                string first = name( node );
                string second = name( ref );
                if ( second < first )
                    swap( first, second );
                nodes.insert( make_pair(
                    first,
                    second
                ) );

                labels[ name( node ) ] = label( node );
                labels[ name( ref ) ] = label( ref );
            }
        }
        return true;
    };
    pulse( root, on_node );

    plot_dot( nodes, labels );
}

