#include "catch.hpp"
#include "pcc.hpp"

TEST_CASE( "int", "[!shouldfail]" ) {
    pcc::run_test( []( int x ) {
            REQUIRE( x );
       } );
}

TEST_CASE( "pair fst", "[!shouldfail]" ) {
    pcc::run_test( []( std::pair< int, unsigned > x ) {
            REQUIRE( x.first >= 0 );
       } );
}

TEST_CASE( "pair snd", "[!shouldfail]" ) {
    pcc::run_test( []( std::pair< int, unsigned > x ) {
            REQUIRE( x.second == 0 );
       } );
}

TEST_CASE( "tuple" ) {
    pcc::run_test( []( std::tuple< int, int, int > t ) {
            auto [a, b, c] = t;
            REQUIRE( (a + b) + c == a + (b + c) );
        } );
}

TEST_CASE( "multiple args" ) {
    pcc::run_test( []( int a, int b, int c ) {
            REQUIRE( (a + b) + c == a + (b + c) );
        } );
}

TEST_CASE( "args of different types" ) {
    pcc::run_test( []( int a, std::tuple< long, unsigned > bc, size_t d ) {
            auto [b, c] = bc;
            REQUIRE( (a + b) + (c + d) == a + (b + (c + d)) );
        } );
}

TEST_CASE( "tuple float", "[!shouldfail]" ) {
    pcc::run_test( []( std::tuple< float, float, float > t ) {
            auto [a, b, c] = t;
            REQUIRE( (a + b) + c == a + (b + c) );
        } );
}

TEST_CASE( "multiarg float", "[!shouldfail]" ) {
    pcc::run_test( []( float a, float b, float c ) {
            REQUIRE( (a + b) + c == a + (b + c) );
        } );
}

TEST_CASE( "skip test bound", "[!shouldfail]" ) {
    pcc::config c;
    c.max_fail_ratio = 2;
    c.num_tests = 2;
    pcc::run_test( []( int ) {
            throw pcc::skip_test();
        }, c );
}

TEST_CASE( "skip test" ) {
    pcc::run_test( []( int x ) {
            if ( x % 2 != 0 )
                throw pcc::skip_test();
            REQUIRE( x % 2 == 0 );
        } );
}

struct Custom {
    int a;
    int b;
    bool c = false;
};

auto arbitrary( pcc::witness< Custom > ) {
    return []( pcc::generator &gen ) {
        auto iarb = pcc::arbitrary< int >();
        Custom c;
        c.a = iarb( gen );
        c.b = iarb( gen );
        c.c = true;
        return c;
    };
}

TEST_CASE( "custom" ) {
    pcc::run_test( []( Custom c ) {
            REQUIRE( c.c );
        } );
}

TEST_CASE( "fun noncomutative", "[!shouldfail]" ) {
    pcc::run_test( []( pcc::fun< int( int, int ) > f, int a, int b ) {
        REQUIRE( f( a, b ) == f( b, a ) );
    } );
}
