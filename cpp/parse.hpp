
auto source = R"V0G0N(
k = 100
x = 2
y = 3
b = 4
)V0G0N";

#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>

namespace qi = boost::spirit::qi;
namespace phx = boost::phoenix;
using namespace std;

/******************************************************************************/

// Utility to run a parser, check for errors, and capture the results.
template <typename Parser, typename Skipper, typename ... Args>
void PhraseParseOrDie(
    const std::string& input,
    const Parser& p,
    const Skipper& s,
    Args && ... args
)
{
    std::string::const_iterator begin = input.begin(), end = input.end();
    boost::spirit::qi::phrase_parse(
        begin,
        end,
        p,
        s,
        std::forward< Args >( args ) ...
    );
    if ( begin != end ) {
        std::cout << "Unparseable: "
                  << std::quoted( std::string( begin, end ) ) << std::endl;
        throw std::runtime_error( "Parse error" );
    }
}

#define Unit qi::rule< string::const_iterator, string(), qi::space_type >

void test1( const auto & input )
{
    Unit func_name, term_name, expr, file;
    try {
        //varname %= qi::alpha >> * qi::alnum;
        term_name = ( + qi::alnum );
        //term_name %= qi::alpha >> ( * qi::alnum );
        term_name[ ( [&](){cout<<"term_name"<<endl;} ) ];

        func_name = term_name;
        func_name[ ( [&](){cout<<"func_name"<<endl;} ) ];

        expr = func_name > "=" > ( + term_name );
        expr[ ( [&](){cout<<"expr"<<endl;} ) ];

        file = * qi::eol > * ( expr | qi::eol );
        file[ ( [&](){cout<<"file"<<endl;} ) ];

        qi::grammar< std::string::const_iterator, string(), qi::space_type > grammar( file );
        PhraseParseOrDie( input, grammar, qi::space );
    }
    catch ( std::exception& e ) {
        std::cout << "EXCEPTION: " << e.what() << std::endl;
    }
}

/******************************************************************************/

auto parse()
{
    std::string input = source;
    
    cout << "Parsing the source:" << endl << source << endl;
    test1( input );

    cout << "Done." << endl;
}

/*std::ifstream ifs("input.txt" );
ifs.unsetf(std::ios::skipws);
boost::spirit::istream_iterator f(ifs), l;
problem_parser<boost::spirit::istream_iterator> p;*/

/******************************************************************************/
