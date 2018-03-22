/* (c) 2018 Vladimír Štill <xstill@mail.muni.cz>
 *
 * Property testing for Catch2.
 *
 * pcc::run_test( []( int a, std::vector< float > b ) {
 *        // test, using Catch2 assertion macros
 *        // a and b will be randomly generated
 *    } );
 *
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <functional>
#include <type_traits>
#include <random>
#include <vector>
#include <optional>
#include <tuple>
#include <sstream>

#ifndef PCC_HPP
#define PCC_HPP

namespace pcc {

struct generator : std::mt19937_64
{
    using std::mt19937_64::mt19937_64;

    generator( std::mt19937_64::result_type seed, size_t size ) :
        std::mt19937_64( seed ), size( size )
    { }

    generator( const generator & ) = delete;
    generator( generator && ) = default;

    size_t size = 20;
};

struct config
{
    generator get_rng() {
        if ( seed )
            return generator( *seed, max_size );
        std::random_device rd;
        return generator( rd(), max_size );
    }

    int num_tests = 1000;
    int max_fail_ratio = 10;
    int max_size = 20;
    int max_shrinks = 100;
    std::optional< uint64_t > seed;
};

struct skip_test { };

template< typename T >
struct witness { using type = T; };

template< typename T >
auto arbitrary() {
    return arbitrary( witness< T >() );
}

namespace detail_shrink {
    // this has to be in different namespace than run to avoid prefering
    // shrinks is this namespace

    struct Prefered { };
    struct NotPrefered {
        NotPrefered( Prefered ) { } // NOLINT
    };

    template< typename T >
    auto shrink( Prefered ) -> decltype( shrink( witness< T >() ) ) {
        return shrink( witness< T >() );
    }

    template< typename T >
    struct NullRange {
        using value_type = T;
        struct iterator {
            using value_type = T;
            bool operator==( const iterator & ) const { return true; }
            bool operator!=( const iterator & ) const { return false; }
            iterator &operator++() { return *this; }
            value_type &operator*() { std::abort(); }
        };
        using const_iterator = iterator;

        iterator begin() const { return { }; }
        iterator end() const { return { }; }
    };

    template< typename T >
    auto shrink( NotPrefered ) {
        return []( const T & ) { return NullRange< T >(); };
    }
} // namespace detail_shrink

template< typename T >
auto shrink() {
    return detail_shrink::shrink< T >( detail_shrink::Prefered() );
}

namespace detail {

    struct Bool {
        struct Shrink {
            std::vector< bool > operator()( bool v ) const {
                if ( v )
                    return { false };
                return { };
            }
        };

        struct Arbitrary {
            bool operator()( generator &g ) const {
                std::uniform_int_distribution< int > distr( 0, 1 );
                return distr( g );
            }
        };
    };

    template< typename N >
    struct Num {
        struct Shrink {
            std::vector< N > operator()( N v ) const {
                std::vector< N > out;
                if ( v == 0 )
                    return out;

                out.push_back( 0 );
                if ( v > 1 )
                    out.push_back( v - 1 );
                if constexpr ( std::is_signed_v< N > )
                    if ( v < -1 )
                        out.push_back( v + 1 );
                auto half = v / 2;
                if ( out.back() != half )
                    out.push_back( half );
                return out;
            }
        };
    };

    template< typename I >
    struct Int : Num< I >
    {
        struct Arbitrary {
            I operator()( generator &g ) const {
                if ( g.size == 0 )
                    return I();
                I min = 0;
                if constexpr ( std::is_signed_v< I > ) {
                    min = -I( g.size );
                }
                std::uniform_int_distribution< I > distr( min, g.size );
                return distr( g );
            }
        };
    };

    template< typename F >
    struct Float : Num< F >
    {
        struct Arbitrary {
            F operator()( generator &g ) const {
                if ( g.size == 0 )
                    return F();
                std::uniform_real_distribution< F > distr( -F( g.size ), F( g.size ) );
                return distr( g );
            }
        };
    };

    template< typename I >
    using integral_arbitrary = std::enable_if_t< std::is_integral_v< I >,
                                                 typename Int< I >::Arbitrary >;

    template< typename I >
    using integral_shrink = std::enable_if_t< std::is_integral_v< I >,
                                              typename Int< I >::Shrink >;

    template< typename I >
    using floating_arbitrary = std::enable_if_t< std::is_floating_point_v< I >,
                                                 typename Float< I >::Arbitrary >;

    template< typename I >
    using floating_shrink = std::enable_if_t< std::is_floating_point_v< I >,
                                              typename Float< I >::Shrink >;

    template< typename T, typename Prop, typename Args >
    void run_shrink_loop( witness< T >, Prop &prop, Args args, config &conf ) {
        INFO( "shrinking" );
        auto shk = shrink< T >();
        for ( int count = 0; count < conf.max_shrinks; ++count ) {
            auto shrinked = shk( args );
            if ( shrinked.begin() == shrinked.end() ) { // shrinking done
                CAPTURE( args );
                FAIL( "shrinking done" );
            }
            bool found = false;
            for ( auto &shink_args : shrinked ) {
                try {
                    std::apply( prop, shink_args );
                } catch ( Catch::TestFailureException & ) {
                    found = true;
                    args = std::move( shink_args );
                    break;
                }
            }
            if ( !found ) {
                CAPTURE( args );
                FAIL( "shrinking done" );
            }
        }
    }

    template< typename T, typename Prop >
    void run_t( witness< T > w, Prop &prop, generator &g, config &conf ) {
        auto gen = arbitrary< T >();
        T args = gen( g );
        CAPTURE( args );

        try {
            std::apply( prop, args );
        } catch ( Catch::TestFailureException & ) {
            if constexpr ( std::is_copy_constructible_v< T > && std::is_copy_assignable_v< T > ) {
                run_shrink_loop( w, prop, args, conf );
            } else {
                throw;
            }
        }
    }

    template< typename Cls, typename R, typename... Ts, typename Prop >
    void run( witness< R (Cls::*)( Ts... ) >, Prop &prop, generator &g, config &conf ) {
        run_t( witness< std::tuple< Ts... > >(), prop, g, conf );
    }

    template< typename Cls, typename R, typename... Ts, typename Prop >
    void run( witness< R (Cls::*)( Ts... ) const >, Prop &prop, generator &g, config &conf ) {
        run_t( witness< std::tuple< Ts... > >(), prop, g, conf );
    }

    template< typename R, typename... Ts, typename Prop >
    void run( witness< R (*)( Ts... ) >, Prop &prop, generator &g, config &conf ) {
        run_t( witness< std::tuple< Ts... > >(), prop, g, conf );
    }
} // namespace detail

typename detail::Bool::Arbitrary arbitrary( witness< bool > ) { return { }; }
typename detail::Bool::Shrink shrink( witness< bool > ) { return { }; }

template< typename I >
detail::integral_arbitrary< I > arbitrary( witness< I > ) { return { }; }
template< typename I >
detail::integral_shrink< I > shrink( witness< I > ) { return { }; }

template< typename F >
detail::floating_arbitrary< F > arbitrary( witness< F > ) { return { }; }
template< typename F >
detail::floating_shrink< F > shrink( witness< F > ) { return { }; }

template< typename T, typename = decltype( arbitrary< T >() ) >
auto arbitrary( witness< std::vector< T > > ) {
    return []( generator &g ) {
        auto inner = arbitrary< T >();
        std::vector< T > vec;
        vec.reserve( g.size );
        for ( size_t i = 0; i < vec.capacity(); ++i )
            vec.emplace_back( inner( g ) );
        return vec;
    };
}

template< typename... Ts >
auto arbitrary( witness< std::tuple< Ts... > > ) {
    return []( generator &g ) {
        return std::tuple< Ts... >{ arbitrary< Ts >()( g )... };
    };
}

template< typename A, typename B >
auto arbitrary( witness< std::pair< A, B > > ) {
    return []( generator &g ) {
        return std::pair< A, B >{ arbitrary< A >()( g ),
                                  arbitrary< B >()( g ) };
    };
}

namespace detail {
    template< size_t... Is >
    auto index_tuple( std::index_sequence< Is... > ) {
        return std::tuple( std::integral_constant< size_t, Is >()... );
    }

    template< typename... Ts >
    auto index_tuple() {
        return index_tuple( std::index_sequence_for< Ts... >() );
    }

    template< size_t I, typename Tuple, typename Out, typename Shrinks >
    void shrink_tuple_part( std::integral_constant< size_t, I >, const Tuple &orig, Out &out, Shrinks &shrinks ) {
        for ( auto &v : std::get< I >( shrinks )( std::get< I >( orig ) ) ) {
            auto t = orig;
            std::get< I >( t ) = v;
            out.push_back( std::move( t ) );
        }
    }
} // namespace detail

template< typename... Ts >
auto shrink( witness< std::tuple< Ts ... > > ) {
    std::tuple shrinks( shrink< Ts >()... );
    return [&shrinks]( const std::tuple< Ts ... > &orig ) {
        std::vector< std::tuple< Ts ... > > out;
        std::apply( [&]( auto ... idxs ) {
                (detail::shrink_tuple_part( idxs, orig, out, shrinks ), ...);
            }, detail::index_tuple< Ts... >() );
        return out;
    };
}

template< typename T >
auto shrink( witness< std::vector< T > > ) {
    return []( const std::vector< T > &orig ) {
        std::vector< std::vector< T > > out;
        if ( orig.empty() )
            return out;

        out.push_back( orig );
        out.back().pop_back();
        if ( orig.size() > 1 ) {
            out.push_back( orig );
            out.back().erase( out.back().begin() );
        }
        auto shk = shrink< T >();
        for ( size_t i = 0; i < orig.size(); ++i ) {
            for ( auto &v : shk( orig[ i ] ) ) {
                out.push_back( orig );
                out.back()[i] = v;
            }
        }
        return out;
    };
}

namespace hash {

size_t combine( size_t a, size_t b ) {
    return a ^ (b + 0x9e3779b9 + (a << 6) + (a >> 2));
}

template< typename T >
struct hasher : std::hash< T > { };

template< typename A, typename B >
struct hasher< std::pair< A, B > >
{
    size_t operator()( const std::pair< A, B >& p ) const {
        return combine( hasher< A >()( p.first ), hasher< B >()( p.second ) );
    }
};

template< typename... Args >
struct hasher< std::tuple< Args... > >
{
    static constexpr size_t arity = sizeof...( Args );

    size_t operator()( const std::tuple< Args... > &v ) const {
        return std::apply( [&]( auto... idxs ) {
                std::array< size_t, arity > hashes = { {
                    std::get< idxs.value >( _hashers )( std::get< idxs.value >( v ) )... } };
                return std::accumulate( hashes.begin(), hashes.end(), 0, combine );
            },
            detail::index_tuple< Args... >() );
    }

    std::tuple< hasher< Args >... > _hashers;
};

}

template< typename >
struct fun;

template< typename R, typename... Args >
struct fun< R ( Args... ) >
{
    using tuple = std::tuple< Args... >;
    using table = std::unordered_map< tuple, R, hash::hasher< tuple > >;

    fun( R def, table tab ) : _def( def ), _table( tab ) { }

    R operator()( const Args &...args ) const {
        if ( auto it = _table.find( tuple( args... ) ); it != _table.end() )
            return it->second;
        return _def;
    }

  private:
    friend struct Catch::StringMaker< pcc::fun< R( Args... ) > >;
    R _def;
    table _table;
};

template< typename R, typename... Args,
          typename = std::void_t< decltype( arbitrary< R >() ),
                                  decltype( arbitrary< Args >() )...,
                                  decltype( std::hash< Args >() )... > >
auto arbitrary( witness< fun< R ( Args... ) > > ) {
    return []( generator &g ) {
        auto ar = arbitrary< R >();
        using F = fun< R ( Args ... ) >;
        using Tup = typename F::tuple;
        using Tab = typename F::table;
        auto at = arbitrary< Tup >();
        hash::hasher< Tup > hash;
        Tab table;

        for ( size_t i = 0; i < g.size; ++i ) {
            auto in = at( g );
            auto h = hash( in );
            g.seed( hash::combine( h, g() ) );
            auto r = ar( g );
            table[ in ] = r;
        }

        return fun< R ( Args... ) >( ar( g ), std::move( table ) );
    };
}

template< typename Property >
void run_test( Property prop, config conf ) {
    auto g = conf.get_rng();
    int total_runs = 0;
    for ( int i = 0; i < conf.num_tests; ++i ) {
        g.size = std::ceil( double(i * conf.max_size) / std::max( 1, conf.num_tests - 1 ) );

      retry:
        if ( total_runs > conf.num_tests * conf.max_fail_ratio ) {
            FAIL( "too many skipped tests" );
        }
        try {
            ++total_runs;
            detail::run( witness< decltype( &Property::operator() ) >(), prop, g, conf );
        } catch ( skip_test & ) {
            goto retry;
        }
    }
}

template< typename Property >
void run_test( Property &&prop ) {
    run_test( std::forward< Property >( prop ), config() );
}

template< typename Property >
void run_test_det( Property &&prop ) {
    config c;
    c.seed = 0;
    run_test( std::forward< Property >( prop ), c );
}

namespace detail {

    struct TupleToString
    {
        template< typename... Ts >
        static void run( const std::tuple< Ts... > &t, std::stringstream &ss ) {
            ss << "(";
            size_t i = 0;
            constexpr size_t end = sizeof...( Ts );
            auto dump = [&]( const auto &v ) {
                run( v, ss );
                if ( ++i != end )
                    ss << ", ";
            };
            std::apply( [&]( auto &... vals ) { (dump( vals ), ...); }, t );
            ss << ")";
        }

        template< typename A, typename B >
        static void run( const std::pair< A, B > &t, std::stringstream &ss ) {
            ss << "(";
            run( t.first, ss );
            ss << ", ";
            run( t.second, ss );
            ss << ")";
        }

        template< typename T >
        static void run( const T &v, std::stringstream &ss ) {
            ss << Catch::StringMaker< T >::convert( v );
        }
    };
} // namespace detail

} // namespace pcc

namespace Catch {
    template< typename... Ts >
    struct StringMaker< std::tuple< Ts... > > {
        static std::string convert( const std::tuple< Ts... > &value ) {
            std::stringstream ss;
            pcc::detail::TupleToString::run( value, ss );
            return ss.str();
        }
    };

    template< typename A, typename B >
    struct StringMaker< std::pair< A, B > > {
        static std::string convert( const std::pair< A, B > &value ) {
            std::stringstream ss;
            pcc::detail::TupleToString::run( value, ss );
            return ss.str();
        }
    };

    template< typename R, typename... Args >
    struct StringMaker< pcc::fun< R( Args... ) > > {
        static std::string convert( const pcc::fun< R( Args... ) > f ) {
            std::stringstream ss;
            ss << "{";
            for ( auto & [args, v] : f._table ) {
                ss << StringMaker< std::decay_t< decltype( args ) > >::convert( args )
                   << " -> " << StringMaker< R >::convert( v ) << ", ";
            }
            ss << "_ -> " << StringMaker< R >::convert( f._def ) << "}";
            return ss.str();
        }
    };
} // namespace Catch

#endif // PCC_HPP
