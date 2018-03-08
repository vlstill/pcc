# Property Checking for Catch2

Header-only library for property checking in C++17 with the
[Catch2](https://github.com/catchorg/Catch2) unit testing framework.
This library is inspire by the great Haskell testing library
[QuickCheck](https://hackage.haskell.org/package/QuickCheck/docs/Test-QuickCheck.html).
It uses random-generated data which it feeds to unit tests in order to discover
property/invariant violations.

It is best to start with an example:

```c++
#include <catch.hpp>
#include <pcc.hpp>

TEST_CASE( "pcc example" ) {
    pcc::run_test( []( int a, std::vector< float > b ) {
        // test, using Catch2 assertion macros
        // a and b will be randomly generated
        REQUIRE( a == 0 ); // this will fail
    } );
}
```

Here we see normal Catch2 test named `"pcc example"` which contains call to
`pcc`'s entry point `run_test`. `run_test` accepts a callable object (which must
have non-overloaded call operator) and it generates random instances of
arguments for this callable object. The number of tests and size of instances
can be configured by passing `pcc::config` as a second argument (see source for
`pcc::config`.
