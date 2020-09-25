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

bool try_use(
    Node * node,
    const map< Node *, int64_t > & evaluated,
    int64_t & result
) {
    if ( node->type( TYPE::TERM ) ) {
        if ( node->type( TYPE::NUMBER ) ) {
            result = stol( node->content );
            return true;
        }
        throw runtime_error( "ERROR: undefined term" );
    }
    //other expression:
    auto ev = evaluated.find( node );
    if ( ev == evaluated.end() )
        return false;
}

/** Extract left and right nodes of specified operator.*/
bool extract( Node * op, Node *& left, Node *& right ) {
    const auto Operand = Operands.at( op->content );
    left = nullptr;
    right = nullptr;
    switch ( Operand ) {
        case OPERAND::INFIX:
            for ( auto & ref : op->refs ) {
                if ( ref->type( set{ TYPE::EXPRESSION, TYPE::TERM } ) )
                    left = ref;
                else if ( ref->type( TYPE::NONABELIAN ) )
                    right = * ref->refs.begin();
            }
            if ( left == nullptr || right == nullptr ) {
                stringstream s;
                s << "ERROR: couldn't resolve left and right operands of operator " << op;
                cout << s.str() << endl;
                throw runtime_error( s.str() );
            }
            break;
        
        case OPERAND::LEFT:
        case OPERAND::RIGHT:
            left = find_types( op->refs, set{ TYPE::EXPRESSION, TYPE::TERM } );
            break;
        case OPERAND::RIGHT_ALL:
            //TODO:
            break;
        default:
            stringstream s;
            s << "ERROR: unresolved operator's operand type " << op;
            cout << s.str() << endl;
            throw runtime_error( s.str() );
            break;
    }
}

bool try_evaluate(
    Node * expr,
    const map< Node *, int64_t > & evaluated,
    int64_t & value
) {
    auto op = find_types( expr->refs, TYPE::OPERATOR );
    if ( op == nullptr )
        throw runtime_error( "ERROR: Expected operator" );
    
    Node * left = nullptr;
    Node * right = nullptr;
    extract( op, left, right );

    auto left_it = evaluated.find( left );
    if ( left != nullptr && left_it == evaluated.end() )
        return false;
    auto right_it = evaluated.find( right );
    if ( right != nullptr && right_it == evaluated.end() )
        return false;
    
    if      ( op->content == "/"  ) {
        value = left_it->second / right_it->second;
    }
    else if ( op->content == "+"  ) {
        value = left_it->second / right_it->second;
    }
    else if ( op->content == "-"  ) {
        value = left_it->second - right_it->second;
    }
    else if ( op->content == "*"  ) {
        value = left_it->second * right_it->second;
    }
    else if ( op->content == "@+" ) {
        value = left_it->second + right_it->second;
    }
    else {
        throw runtime_error( string( "ERROR: undefined yet operator: " ) + op->content );
    }

    return true;
}

void try_evaluate_all( Node * root ) {
    //TODO: debug mode ofc, until evaluation is sufficiently abstract:
    map< Node *, int64_t > evaluated;

    //everything pulses once:
    bool moved = true;
    while ( moved ) {
        moved = false;
        const auto & on_expr = [&]( Node * expr ) {
            if ( ! expr->type( TYPE::EXPRESSION ) )
                return;

            //if every TERM this EXPRESSION depends on can be evaluated right now:
            bool free = true;
            for ( auto & ref : expr->refs ) {
                if ( ! ref->type( set{ TYPE::EXPRESSION, TYPE::TERM } ) )
                    continue;
                if ( evaluated.find( ref ) == evaluated.end() ) {
                    free = false;
                    break;
                }
            }
            if ( free ) {
                moved = true;
                int64_t value = 0;
                const auto was_evaluated = try_evaluate( expr, evaluated, value );
                if ( was_evaluated ) {
                    evaluated[ expr ] = value;
                }
            }
        };
        pulse( root, on_expr );
    }
    //every node should be evaluated by now because we're topologically locked (knotted?) ...
}

void intersect( Node * root ) {
    map< Node *, list< set< Node * > > > state;
}

auto semantic( Node * root ) {
    merge_occurences( root );
}
