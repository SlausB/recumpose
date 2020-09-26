#include "syntax_tree.hpp"

/** All yet syntactically the same TERMs should merge into a single TERM (all EXPRESSIONs references need to repoint) and those dropped are removed.*/
void merge_occurences( Node * root ) {
    set< Node * > remove;
    map< string, Node * > terms;

    const auto & on_term = [&]( Node * term ) {
        if ( ! term->type( TYPE::TERM ) )
            return;
        
        auto existing = terms[ term->content ];
        if ( existing == nullptr ) {
            terms[ term->content ] = term;
            return;
        }

        //move all expressions that reference one of these occurences into another one:
        cout << "Merge occurence " << term << " into " << existing << endl;
        for ( auto expr : term->refd ) {
            if ( expr->type( set{ TYPE::EXPRESSION, TYPE::NONABELIAN } ) ) {
                cout << "    " << expr << " -> " << existing << endl;
                expr->ref( existing );
            }
        }
        remove.insert( term );
    };
    pulse( root, on_term );

    for ( auto & r : remove )
        delete r;
}

int64_t string_to_int( const string & content ) {
    char * tail;
    return strtoll( content.c_str(), & tail, 0 );
}

bool try_use(
    Node * node,
    const map< Node *, int64_t > & evaluated,
    int64_t & result
) {
    if ( node->type( TYPE::TERM ) ) {
        if ( node->type( TYPE::NUMBER ) ) {
            result = string_to_int( node->content );
            return true;
        }
        //otherwise it's just a variable and we cannot evaluate it directly:
        return false;
    }
    //other expression:
    auto ev = evaluated.find( node );
    if ( ev == evaluated.end() )
        return false;
    result = ev->second;
    return true;
}

/** Extract left and right nodes of specified EXPRESSION.*/
void extract( Node * expr, Node * op, Node *& left, Node *& right ) {
    const auto Operand = Operands.at( op->content );
    left = nullptr;
    right = nullptr;
    switch ( Operand ) {
        case OPERAND::INFIX:
        {
            for ( auto & ref : expr->refs ) {
                if ( ref->type( set{ TYPE::EXPRESSION, TYPE::TERM } ) ) {
                    if ( left == nullptr )
                        left = ref;
                    else
                        right = ref;
                }
            }

            auto nonabelian = find_types( expr->refs, TYPE::NONABELIAN );
            if ( nonabelian != nullptr )
                right = * nonabelian->refs.begin();
            
            if ( left == nullptr || right == nullptr ) {
                stringstream s;
                s << "ERROR: couldn't resolve left and right operands of " << expr;
                cout << s.str() << endl;
                cout << "    it's refs:" << endl;
                for ( auto & ref : expr->refs )
                    cout << "        " << ref << endl;
                throw runtime_error( s.str() );
            }
            break;
        }
        
        case OPERAND::LEFT:
        case OPERAND::RIGHT:
            left = find_types( expr->refs, set{ TYPE::EXPRESSION, TYPE::TERM } );
            break;
        case OPERAND::RIGHT_ALL:
            //TODO:
            break;
        default:
            stringstream s;
            s << "ERROR: unresolved operand type of operator " << op;
            cout << s.str() << endl;
            throw runtime_error( s.str() );
    }
}

auto apply_operator( Node * op, const auto & left, const auto & right )
{
    if      ( op->content == "/"  ) {
        return left / right;
    }
    else if ( op->content == "+"  ) {
        return left / right;
    }
    else if ( op->content == "-"  ) {
        return left - right;
    }
    else if ( op->content == "*"  ) {
        return left * right;
    }
    else if ( op->content == "@+" ) {
        return left + right;
    }
    
    throw runtime_error( string( "ERROR: undefined yet operator: " ) + op->content );
}

bool is_free_to_evaluate( Node * expr, const auto & evaluated ) {
    for ( auto & ref : expr->refs ) {
        if ( ! ref->type( set{ TYPE::EXPRESSION, TYPE::TERM } ) )
            continue;
        if ( evaluated.find( ref ) == evaluated.end() )
            return false;
    }
    return true;
}

bool try_evaluate(
    Node * expr,
    const map< Node *, int64_t > & evaluated,
    int64_t & value
) {
    if ( try_use( expr, evaluated, value ) )
        return true;
    
    //if it's TERM and still wasn't used, then it's yet undefined variable:
    if ( expr->type( TYPE::TERM ) )
        return false;

    auto op = find_types( expr->refs, TYPE::OPERATOR );
    if ( op == nullptr )
        throw runtime_error( "ERROR: Expected operator" );
    
    Node * left = nullptr;
    Node * right = nullptr;
    extract( expr, op, left, right );

    int64_t left_v;
    if ( left != nullptr ) {
        if ( ! try_use( left, evaluated, left_v ) )
            return false;
    }
    int64_t right_v;
    if ( right != nullptr ) {
        if ( ! try_use( right, evaluated, right_v ) )
            return false;
    }
    
    apply_operator( op, left_v, right_v );

    return true;
}

void try_evaluate_all( Node * root ) {
    //TODO: debug mode ofc, until evaluation is sufficiently abstract:
    map< Node *, int64_t > evaluated;

    //everything pulses once:
    bool moved = true;
    while ( moved ) {
        cout << "Trying to evaluate any of expressions ..." << endl;
        moved = false;
        const auto & on_expr = [&]( Node * expr ) {
            if ( ! expr->type( set{ TYPE::EXPRESSION, TYPE::TERM } ) )
                return;
            
            //if was already evaluated within current propagation:
            if ( evaluated.find( expr ) != evaluated.end() )
                return;

            //if every TERM this EXPRESSION depends on can be evaluated right now:
            if ( is_free_to_evaluate( expr, evaluated ) ) {
                int64_t value = 0;
                const auto was_evaluated = try_evaluate( expr, evaluated, value );
                if ( was_evaluated ) {
                    cout << "    expression " << expr << " got evaluated." << endl;
                    moved = true;
                    evaluated[ expr ] = value;
                }
            }
        };
        pulse( root, on_expr );
    }
    //every node should be evaluated by now because we're topologically locked (knotted?) ...
    cout << "Topo evaluation has stopped due to lock or exhaustion. Nodes evaluated: " << evaluated.size() << endl;
}

void intersect( Node * root ) {
    map< Node *, list< set< Node * > > > state;
}

auto semantic( Node * root ) {
    cout << "SEMANTIC:" << endl;
    merge_occurences( root );
    try_evaluate_all( root );
}
