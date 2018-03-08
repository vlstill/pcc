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
`pcc::config`).

## Main Features

- automatic generation of inputs based o type of the tested function/property
- shrinking of counterexamples -- when test fails `pcc` will try to find smaller
  input on which the test also fails
- header only
- integrates with Catch2 -- uses Catch2 assertions
- written for C++17, there will probably never be support for GCC < 7 and Clang < 5
- support for generators and shrinks for custom types

## Known Limitations

- `SECTIONS` do not work inside `run_test`
- failures not from Catch2 macros will not result in shrinking of inputs
- shrinking is noisy -- it shows intermediate results during shrinking
