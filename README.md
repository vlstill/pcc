# Property Checking for Catch2

Header-only library for property checking in C++17 with the
[Catch2](https://github.com/catchorg/Catch2) unit testing framework.

```c++
#include <catch.hpp>
#include <pcc.hpp>

TEST_CASE( "PCC example" ) {
    pcc::run_test( []( int a, std::vector< float > b ) {
        // test, using Catch2 assertion macros
        // a and b will be randomly generated
        REQUIRE( a == 0 ); // this will fail
    } );
```
