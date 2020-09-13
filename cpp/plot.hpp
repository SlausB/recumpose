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
    string file_name;
    string graph_name;

    Plot(
        TYPE type = TYPE::DIRECTED,
        const string & file_name = string( "graph.dot" ),
        const string & graph_name = string( "recumpose" )
    ): type(type), file_name(file_name), graph_name(graph_name)  {
        o.open( file_name );
        o << ( type == TYPE::DIRECTED ? "digraph " : "graph " ) << graph_name << " {" << endl;
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
        const string command = string( "dot -Tpng " ) + file_name + " -o " + graph_name + ".png";
        system( command.c_str() );
    }
    auto f() {
        return type == TYPE::DIRECTED ? " -> " : " -- ";
    }
};

auto name( Node * node ) {
    return (uint64_t) (void const *) node;
};

auto plot( Node * root ) {
    Plot p;
    map< uint64_t, string > labels;

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
                p.relation( name( node ), name( ref ) );
                
                labels[ name( node ) ] = label( node );
                labels[ name( ref  ) ] = label( ref  );
            }
        }
        return true;
    };
    pulse( root, on_node );

    for ( const auto & label : labels ) {
        p.label( label.first, label.second );
    }
}

