#pragma once

#include <string>
#include <set>
#include <iostream>
#include <fstream>
#include <cstdlib>
using namespace std;

/** DOT language output utility.*/
struct Plot {
    enum TYPE {
        UNDIRECTED,
        DIRECTED,
    };
    TYPE type;

    ofstream o;
    string name;

    Plot(
        TYPE type = TYPE::DIRECTED,
        const string & name = string( "graph" )
    ): type(type), name(name) {
        o.open( name + ".dot" );
        o << ( type == TYPE::DIRECTED ? "digraph " : "graph " ) << name << " {" << endl;
    }

    void relation( const auto & from, const auto & to ) {
        o << "    " << from << f() << to << ";" << endl;
    }
    /** Redefine displayed name of id node.*/
    void label( const auto & id, const auto & name ) {
        o << "    " << id << " [label=\"" << name << "\"];" << endl;
    }

    ~Plot() {
        o << "}" << endl;
        const string command = string( "dot -Tpng " ) + name + ".dot" + " -o " + name + ".png";
        system( command.c_str() );
    }
    auto f() {
        return type == TYPE::DIRECTED ? " -> " : " -- ";
    }
};

auto name( Node * node ) {
    return (uint64_t) (void const *) node;
};

template< typename PlotTypes >
auto plot(
    Node * root,
    const PlotTypes & plot_types = {},
    const auto & graph_name = "graph"
) {
    Plot p( Plot::TYPE::DIRECTED, graph_name );
    map< uint64_t, string > labels;

    const auto & should_plot = [&]( Node * node ) {
        return node->type( plot_types );
    };
    const auto & label = [&]( Node * node ) {
        if ( node->type( TYPE::LINE ) )
            return string( "line " ) + to_string( node->source_pos.line );
        else
            return node->content;
    };

    const auto & on_node = [&]( Node * node ) {
        if ( ! should_plot( node ) )
            return;
        for ( const auto & ref : node->refs ) {
            if ( ! should_plot( ref ) )
                continue;
            
            p.relation( name( node ), name( ref ) );
            
            labels[ name( node ) ] = label( node );
            labels[ name( ref  ) ] = label( ref  );
        }
    };
    pulse( root, on_node );

    for ( const auto & label : labels ) {
        p.label( label.first, label.second );
    }
}

