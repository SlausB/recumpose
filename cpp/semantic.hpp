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

double string_to_int( const string & content ) {
    char * tail;
    return strtoll( content.c_str(), & tail, 0 );
}

/** Obtain the result of evaluation of specified node and return true if successfully obtained.*/
bool try_use(
    Node * node,
    const map< Node *, double > & evaluated,
    double & result
) {
    if ( node->type( TYPE::TERM ) ) {
        if ( node->type( TYPE::NUMBER ) ) {
            result = string_to_int( node->content );
            return true;
        }
    }
    //other expression or variables:
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
                    if ( right == nullptr )
                        right = ref;
                    else
                        left = ref;
                }
            }

            auto nonabelian = find_types( expr->refs, TYPE::NONABELIAN );
            if ( nonabelian != nullptr ) {
                auto na = * nonabelian->refs.begin();
                if ( na == right )
                    swap( left, right );
            }
            
            if ( left == nullptr || right == nullptr ) {
                stringstream s;
                s << "ERROR: couldn't resolve left and right operands of " << expr;
                cout << s.str() << endl;
                cout << "    it's refs:" << endl;
                for ( auto & ref : expr->refs )
                    cout << "        " << ref << endl;
                throw runtime_error( s.str() );
            }
            if ( left == right ) {
                stringstream s;
                s << "ERROR: left and right operands of " << expr << " are the same; something is wrong";
                cout << s.str() << endl;
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
    if ( op->content == "/"  )
        return left / right;
    if ( op->content == "+"  )
        return left / right;
    if ( op->content == "-"  )
        return left - right;
    if ( op->content == "*"  )
        return left * right;
    if ( op->content == "@+" )
        return left + right;
    
    throw runtime_error( string( "ERROR: undefined yet operator: " ) + op->content );
}

void check_bidirectional_op(
    const Node * op,
    const bool can_left,
    const bool can_right,
    const Node * left,
    const Node * right
) {
    if ( can_left && can_right ) {
        stringstream s;
        s << "ERROR: both left and right operands of " << op << " are already evaluated: shouldn't evaluate it on top; something is wrong.";
        cout << s.str() << endl;
        throw new runtime_error( s.str() );
    }

    if ( left == nullptr && right == nullptr ) {
        stringstream s;
        s << "ERROR: bidirectional operator " << op << " requires both left and right operands.";
        cout << s.str() << endl;
        throw new runtime_error( s.str() );
    }
}

/**
@param destination where the result of evaluation should go. Is set to any non-null value only if goes into one of OPERATOR's TERMs (like "=", "<-" ...).
*/
bool try_evaluate(
    Node * expr,
    const map< Node *, double > & values,
    double & value,
    Node *& destination
) {
    destination = expr;

    //if already evaluated or can be (i.e. it's just number) evaluated as it is:
    if ( try_use( expr, values, value ) )
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

    double left_v;
    bool can_left = left != nullptr && try_use( left, values, left_v );

    double right_v;
    bool can_right = right != nullptr && try_use( right, values, right_v );

    if ( ! can_left && ! can_right ) {
        cout << "        cannot evaluate " << expr << " because none of it's both references are evaluated. It's references:" << endl;
        for ( const auto & ref : expr->refs )
            cout << "            " << ref << endl;
        cout << "        left: " << left << endl;
        cout << "        right: " << right << endl;
        return false;
    }

    //some OPERATORs require only one side to be evaluated:
    if ( op->content == "="  ) {
        check_bidirectional_op( op, can_left, can_right, left, right );
        if ( can_left ) {
            value = left_v;
            destination = right;
        }
        else {
            value = right_v;
            destination = left;
        }
        return true;
    }
    if ( op->content == "<-" ) {
        check_bidirectional_op( op, can_left, can_right, left, right );
        if ( can_right ) {
            value = right_v;
            destination = left;
            return true;
        }
        return false;
    }
    if ( op->content == "->" ) {
        check_bidirectional_op( op, can_left, can_right, left, right );
        if ( can_left ) {
            value = left_v;
            destination = right;
            return true;
        }
        return false;
    }

    if ( left != nullptr && ! can_left ) {
        cout << "        left operand of " << expr << " is specified but cannot be evaluated yet." << endl;
        return false;
    }
    if ( right != nullptr && ! can_right ) {
        cout << "        right operand of " << expr << " is specified but cannot be evaluated yet." << endl;
        return false;
    }
    
    //it's free to be evaluated:
    value = apply_operator( op, left_v, right_v );
    return true;
}

void try_evaluate_all( Node * root ) {
    //TODO: debug mode ofc, until evaluation is sufficiently abstract:
    //some EXPRESSIONs require just the flag that it was evaluated, but some require the value:
    set< Node * > evaluated;
    map< Node *, double > values;

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
            
            cout << "    trying to evaluate " << expr << endl;
            
            double value = 0;
            Node * destination;
            if ( ! try_evaluate( expr, values, value, destination ) )
                return;
            cout << "    expression " << expr << " got evaluated." << endl;
            moved = true;
            values[ destination ] = value;
            evaluated.insert( destination );
            if ( expr != destination )
                evaluated.insert( expr );
        };
        pulse( root, on_expr );
    }

    //every node should be evaluated by now because we're topologically locked (knotted?) ...
    cout << "Topo evaluation has stopped due to lock or exhaustion. Nodes evaluated: " << evaluated.size() << ":" << endl;
    cout << "    facts:" << endl;
    for ( const auto & e : evaluated )
        cout << "        " << e << endl;
    cout << "    values:" << endl;
    for ( const auto & v : values )
        cout << "        " << v.first << " : " << v.second << endl;
    
    set< Node * > needs_evaluation;
    const auto & on_ev = [&]( Node * ev ) {
        if ( ev->type( set{ TYPE::EXPRESSION, TYPE::TERM } ) && evaluated.find( ev ) == evaluated.end() )
            needs_evaluation.insert( ev );
    };
    pulse( root, on_ev );
    cout << "    " << needs_evaluation.size() << " need to be evaluated:" << endl;
    for ( const auto & e : needs_evaluation ) {
        cout << "        " << e << endl;
    }
}

void intersect( Node * root ) {
    map< Node *, list< set< Node * > > > state;
}

auto semantic( Node * root ) {
    cout << "SEMANTIC:" << endl;
    merge_occurences( root );
    try_evaluate_all( root );
}
