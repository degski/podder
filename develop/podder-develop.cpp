
// MIT License
//
// Copyright (c) 2018, 2019 degski
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <array>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <random>
#include <sax/iostream.hpp> // <iostream> + nl, sp etc. defined...
#include <set>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <vector>

#include <benchmark/benchmark.h>

#ifdef _WIN32
#    pragma comment( lib, "Shlwapi.lib" )
#endif

#if defined( _DEBUG )
#    pragma comment( lib, "benchmark_maind.lib" )
#    pragma comment( lib, "benchmarkd.lib" )
#else
#    pragma comment( lib, "benchmark_main.lib" )
#    pragma comment( lib, "benchmark.lib" )
#endif

#include <sax/splitmix.hpp>
#include <sax/uniform_int_distribution.hpp>

#include "podder.hpp"

inline std::uint32_t pointer_alignment ( const void * p_ ) {

    return ( std::uint32_t ) ( ( std::uint64_t ) p_ & ( std::uint64_t ) - ( ( std::int64_t ) p_ ) );
}

template<typename T, typename = std::enable_if_t<std::conjunction_v<std::is_integral<T>, std::is_unsigned<T>>>>
std::uint64_t sum ( podder<T> & p ) noexcept {
    std::uint64_t sum = 0;
    for ( auto v : p ) {
        sum += static_cast<std::uint64_t> ( v );
    }
    return sum;
}

template<typename T, typename = std::enable_if_t<std::conjunction_v<std::is_integral<T>, std::is_unsigned<T>>>>
constexpr T sum2n ( const T n_ ) noexcept {
    return ( n_ * ( n_ + 1 ) ) / T ( 2 );
}

template<typename value_type, typename container>
bool podder_emplace_back_type_test ( std::size_t x ) { // test emplace_back

    std::vector<container> vv;
    sax::splitmix64 gen ( x );
    auto dis = sax::uniform_int_distribution<std::size_t> (
        std::size_t{ 0 }, std::min ( static_cast<std::size_t> ( std::numeric_limits<std::uint8_t>::max ( ) - 1 ),
                                     static_cast<std::size_t> ( std::numeric_limits<value_type>::max ( ) - 1 ) ) );
    std::uint64_t a = 0;

    for ( std::size_t i = 0; i < 200; ++i ) {
        vv.emplace_back ( );
        auto const n = dis ( gen );
        for ( std::size_t j = 0; j < n; ++j ) {
            vv.back ( ).emplace_back ( ( value_type ) j );
            a += j;
        }
    }

    std::uint64_t b = 0;

    for ( auto & v : vv ) {
        for ( auto & i : v ) {
            b += i;
        }
    }

    return a == b;
}

bool podder_emplace_back_test_all_types ( std::size_t x ) {

    bool a = true, b = true, c = true, d = true;

    a = podder_emplace_back_type_test<std::uint8_t, podder<std::uint8_t>> ( x );
    b = podder_emplace_back_type_test<std::uint16_t, podder<std::uint16_t>> ( x );
    c = podder_emplace_back_type_test<std::uint32_t, podder<std::uint32_t>> ( x );
    d = podder_emplace_back_type_test<std::uint64_t, podder<std::uint64_t>> ( x );

    return a and b and c and d;
}

bool podder_emplace_back_test ( ) {

    bool result = true;

    for ( std::size_t i = 1; i < 1'000; ++i ) {
        result = result and podder_emplace_back_test_all_types ( i );
    }

    return result;
}

void podder_stride_constructor_test ( ) {
    {
        podder<std::int8_t> p ( 15, 0, 1 );
        for ( auto v : p ) {
            std::cout << ( int ) v << ' ' << nl;
        }
        std::cout << nl;
        p.print_svo ( );
    }
    {
        podder<std::int8_t> p ( 23, 0, -3 );
        for ( auto v : p ) {
            std::cout << ( int ) v << ' ' << nl;
        }
        std::cout << nl;
        p.print_svo ( );
    }
    {
        podder<std::int8_t> p ( 35, 3, 3 );
        for ( auto v : p ) {
            std::cout << ( int ) v << ' ' << nl;
        }
        std::cout << nl;
        p.print_svo ( );
    }
}

template<typename Container>
bool podder_range_constructor_container_test ( ) {

    std::uint64_t a = 0;

    Container v1{ 1, 2, 3, 4, 5 }; // 15
    podder<typename Container::value_type> p1 ( v1.begin ( ), v1.end ( ) );

    a += sum ( p1 );

    Container v2 = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 }; // 120
    podder<typename Container::value_type> p2 ( v2.begin ( ), v2.end ( ) );

    a += sum ( p2 );

    Container v3 = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
                     1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 }; // 480
    podder<typename Container::value_type> p3 ( v3.begin ( ), v3.end ( ) );

    a += sum ( p3 );

    return a == sum2n ( 5u ) + sum2n ( 15u ) + 480;
}

template<typename type>
bool podder_range_constructor_types_test ( ) {

    bool c = podder_range_constructor_container_test<podder<type>> ( );
    bool a = podder_range_constructor_container_test<std::vector<type>> ( );
    bool b = podder_range_constructor_container_test<std::deque<type>> ( );
    bool d = podder_range_constructor_container_test<std::list<type>> ( );

    return a and b and c and d;
}

bool podder_range_constructor_test ( ) {

    bool a = podder_range_constructor_types_test<std::uint8_t> ( );
    bool b = podder_range_constructor_types_test<std::uint16_t> ( );
    bool c = podder_range_constructor_types_test<std::uint32_t> ( );
    bool d = podder_range_constructor_types_test<std::uint64_t> ( );

    return a and b and c and d;
}

bool podder_insert_test ( ) {

    std::uint64_t a = 0;

    // iterator insert ( const_iterator pos, const_reference v_ )
    {
        podder<std::uint16_t> p1{ 2, 3, 5 };
        p1.insert ( p1.begin ( ), 1 );
        p1.insert ( p1.end ( ), 6 );
        std::uint16_t val = 4;
        p1.insert ( p1.begin ( ) + 3, val );
        a += sum ( p1 );

        podder<std::uint16_t> p2{ 2, 3, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
        p2.insert ( p2.end ( ), 16 );
        p2.insert ( p2.begin ( ), 1 );
        p2.insert ( p2.begin ( ) + 3, val );
        a += sum ( p2 );
    }
    // iterator insert ( const_iterator pos, size_type count, const_reference value )
    {
        podder<std::uint16_t> p3{ 1, 2, 3, 4, 5 };
        p3.insert ( p3.begin ( ), 4, 1 );
        a += sum ( p3 );

        podder<std::uint16_t> p4{ 1, 2, 3, 4, 5 };
        p4.insert ( p4.end ( ), 4, 1 );
        a += sum ( p4 );

        podder<std::uint16_t> p5{ 1, 2, 3, 4, 5 };
        p5.insert ( &p5[ 3 ], 4, 1 );
        a += sum ( p5 );

        podder<std::uint16_t> p6{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        p6.insert ( p6.begin ( ), 4, 1 );
        a += sum ( p6 );

        podder<std::uint16_t> p7{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        p7.insert ( p7.end ( ), 4, 1 );
        a += sum ( p7 );

        podder<std::uint16_t> p8{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        p8.insert ( &p8[ 3 ], 4, 1 );
        a += sum ( p8 );

        podder<std::uint16_t> p9{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
        p9.insert ( p9.begin ( ), 4, 1 );
        a += sum ( p9 );

        podder<std::uint16_t> p10{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
        p10.insert ( p10.end ( ), 4, 1 );
        a += sum ( p10 );

        podder<std::uint16_t> p11{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
        p11.insert ( &p11[ 3 ], 4, 1 );
        a += sum ( p11 );

        podder<std::uint16_t> p12{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
        p12.insert ( p12.begin ( ), 4, 1 );
        a += sum ( p12 );

        podder<std::uint16_t> p13{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
        p13.insert ( p13.end ( ), 4, 1 );
        a += sum ( p13 );

        podder<std::uint16_t> p14{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
        p14.insert ( &p14[ 3 ], 4, 1 );
        a += sum ( p14 );
    }
    // iterator insert ( const_iterator pos, InputIt first, InputIt last ) ( contiguous )

    std::vector<std::uint16_t> vec{ 1, 1, 1, 1 };
    {
        podder<std::uint16_t> p15{ 1, 2, 3, 4, 5 };
        p15.insert ( p15.begin ( ), vec.begin ( ), vec.end ( ) );
        a += sum ( p15 );

        podder<std::uint16_t> p16{ 1, 2, 3, 4, 5 };
        p16.insert ( p16.end ( ), vec.begin ( ), vec.end ( ) );
        a += sum ( p16 );

        podder<std::uint16_t> p17{ 1, 2, 3, 4, 5 };
        p17.insert ( &p17[ 3 ], vec.begin ( ), vec.end ( ) );
        a += sum ( p17 );

        podder<std::uint16_t> p18{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        p18.insert ( p18.begin ( ), vec.begin ( ), vec.end ( ) );
        a += sum ( p18 );

        podder<std::uint16_t> p19{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        p19.insert ( p19.end ( ), vec.begin ( ), vec.end ( ) );
        a += sum ( p19 );

        podder<std::uint16_t> p20{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        p20.insert ( &p20[ 3 ], vec.begin ( ), vec.end ( ) );
        a += sum ( p20 );

        podder<std::uint16_t> p21{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
        p21.insert ( p21.begin ( ), vec.begin ( ), vec.end ( ) );
        a += sum ( p21 );

        podder<std::uint16_t> p22{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
        p22.insert ( p22.end ( ), vec.begin ( ), vec.end ( ) );
        a += sum ( p22 );

        podder<std::uint16_t> p23{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
        p23.insert ( &p23[ 3 ], vec.begin ( ), vec.end ( ) );
        a += sum ( p23 );

        podder<std::uint16_t> p24{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
        p24.insert ( p24.begin ( ), vec.begin ( ), vec.end ( ) );
        a += sum ( p24 );

        podder<std::uint16_t> p25{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
        p25.insert ( p25.end ( ), vec.begin ( ), vec.end ( ) );
        a += sum ( p25 );

        podder<std::uint16_t> p26{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
        p26.insert ( &p26[ 3 ], vec.begin ( ), vec.end ( ) );
        a += sum ( p26 );
    }
    // iterator insert ( const_iterator pos, InputIt first, InputIt last ) ( discontiguous )

    std::deque<std::uint16_t> deq{ 1, 1, 1, 1 };
    {
        podder<std::uint16_t> p27{ 1, 2, 3, 4, 5 };
        p27.insert ( p27.begin ( ), deq.begin ( ), deq.end ( ) );
        a += sum ( p27 );

        podder<std::uint16_t> p28{ 1, 2, 3, 4, 5 };
        p28.insert ( p28.end ( ), deq.begin ( ), deq.end ( ) );
        a += sum ( p28 );

        podder<std::uint16_t> p29{ 1, 2, 3, 4, 5 };
        p29.insert ( &p29[ 3 ], deq.begin ( ), deq.end ( ) );
        a += sum ( p29 );
    }
    {
        podder<std::uint16_t> p30{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        p30.insert ( p30.begin ( ), deq.begin ( ), deq.end ( ) );
        a += sum ( p30 );

        podder<std::uint16_t> p31{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        p31.insert ( p31.end ( ), deq.begin ( ), deq.end ( ) );
        a += sum ( p31 );

        podder<std::uint16_t> p32{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        p32.insert ( &p32[ 3 ], deq.begin ( ), deq.end ( ) );
        a += sum ( p32 );
    }
    {
        podder<std::uint16_t> p33{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
        p33.insert ( p33.begin ( ), deq.begin ( ), deq.end ( ) );
        a += sum ( p33 );

        podder<std::uint16_t> p34{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
        p34.insert ( p34.end ( ), deq.begin ( ), deq.end ( ) );
        a += sum ( p34 );

        podder<std::uint16_t> p35{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
        p35.insert ( &p35[ 3 ], deq.begin ( ), deq.end ( ) );
        a += sum ( p35 );
    }
    {
        podder<std::uint16_t> p36{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
        p36.insert ( p36.begin ( ), deq.begin ( ), deq.end ( ) );
        a += sum ( p36 );

        podder<std::uint16_t> p37{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
        p37.insert ( p37.end ( ), deq.begin ( ), deq.end ( ) );
        a += sum ( p37 );

        podder<std::uint16_t> p38{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
        p38.insert ( &p38[ 3 ], deq.begin ( ), deq.end ( ) );
        a += sum ( p38 );
    }
    return a ==
           sum2n ( 6u ) + sum2n ( 16u ) +
               3 * ( 3 * ( sum2n ( 5u ) + 4 ) + 3 * ( sum2n ( 9u ) + 4 ) + 3 * ( sum2n ( 12u ) + 4 ) + 3 * ( sum2n ( 11u ) + 4 ) );
}

void podder_move_assignment_test ( ) {

    using type = std::uint16_t;

    podder<type> p ( 5, 0, 2 );

    for ( auto v : p ) {
        std::cout << ( int ) v << ' ';
    }
    std::cout << nl;
    p.print_svo ( );

    p = podder<type> ( 35, 1, 2 );

    for ( auto v : p ) {
        std::cout << ( int ) v << ' ';
    }
    std::cout << nl;
    p.print_svo ( );

    p = podder<type> ( 15, 1, 3 );

    for ( auto v : p ) {
        std::cout << ( int ) v << ' ';
    }
    std::cout << nl;
    p.print_svo ( );

    p = podder<type> ( 5, 4, 5 );

    for ( auto v : p ) {
        std::cout << ( int ) v << ' ';
    }
    std::cout << nl;
    p.print_svo ( );
}

int podder_assignment_test ( ) {

    std::array<std::uint8_t, 4> a{ 0, 1, 2, 3 };
    std::vector<std::uint8_t> v{ 1, 2, 3, 4 };
    podder<std::uint8_t> q{ 2, 3, 4, 5 };
    std::uint8_t c[ 4 ]{ 3, 4, 5, 6 };
    std::initializer_list<std::uint8_t> i{ 7, 8, 9, 10 };
    std::deque<std::uint8_t> d{ 1, 2, 3, 4 };

    podder<std::uint8_t> p0 ( c );
    print ( p0 );
    podder<std::uint8_t> p1 ( d );
    print ( p1 );

    podder<std::uint8_t> p2 = c;
    print ( p2 );

    return 0;
}

int main343554 ( ) {

    podder<std::uint16_t> p1;
    p1.assign ( 6, 123 );
    print ( p1 );
    podder<std::uint16_t> p2;
    p2.assign ( 16, 123 );
    print ( p2 );
    podder<std::uint16_t> p3{ 1, 2, 3 };
    p3.assign ( 6, 123 );
    print ( p3 );
    podder<std::uint16_t> p4{ 1, 2, 3, 4, 5, 6, 7, 8 };
    p4.assign ( 6, 123 );
    print ( p4 );
    podder<std::uint16_t> p5{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
    p5.assign ( 6, 123 );
    print ( p5 );
    podder<std::uint16_t> p6{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };
    print ( p6 );
    p6.assign ( 6, 123 );
    print ( p6 );

    std::cout << p6.at ( 4 ) << nl;
    std::cout << p6.at ( 1 ) << nl;

    std::cout << p6[ 1 ] << nl;

    return 0;
}

bool podder_comparison_test ( ) {

    bool b = true;

    {
        podder<std::uint16_t> a1{ 1, 2, 3, 4 };
        podder<std::uint16_t> a2{ 1, 2, 3, 4 };

        b &= ( ( ( a1 == a2 ) + ( a1 != a2 ) + ( a1 <= a2 ) + ( a1 >= a2 ) + ( a1 < a2 ) + ( a1 > a2 ) ) == 3 );
        b &= ( ( ( a2 == a1 ) + ( a2 != a1 ) + ( a2 <= a1 ) + ( a2 >= a1 ) + ( a2 < a1 ) + ( a2 > a1 ) ) == 3 );
    }
    {
        podder<std::uint16_t> a1{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
        podder<std::uint16_t> a2{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };

        b &= ( ( ( a1 == a2 ) + ( a1 != a2 ) + ( a1 <= a2 ) + ( a1 >= a2 ) + ( a1 < a2 ) + ( a1 > a2 ) ) == 3 );
        b &= ( ( ( a2 == a1 ) + ( a2 != a1 ) + ( a2 <= a1 ) + ( a2 >= a1 ) + ( a2 < a1 ) + ( a2 > a1 ) ) == 3 );
    }
    {
        podder<std::uint16_t> a1{ 1, 2, 3, 4, 5 };
        podder<std::uint16_t> a2{ 1, 2, 3, 4 };

        b &= ( ( ( a1 == a2 ) + ( a1 != a2 ) + ( a1 <= a2 ) + ( a1 >= a2 ) + ( a1 < a2 ) + ( a1 > a2 ) ) == 3 );
        b &= ( ( ( a2 == a1 ) + ( a2 != a1 ) + ( a2 <= a1 ) + ( a2 >= a1 ) + ( a2 < a1 ) + ( a2 > a1 ) ) == 3 );
    }
    {
        podder<std::uint16_t> a1{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
        podder<std::uint16_t> a2{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };

        b &= ( ( ( a1 == a2 ) + ( a1 != a2 ) + ( a1 <= a2 ) + ( a1 >= a2 ) + ( a1 < a2 ) + ( a1 > a2 ) ) == 3 );
        b &= ( ( ( a2 == a1 ) + ( a2 != a1 ) + ( a2 <= a1 ) + ( a2 >= a1 ) + ( a2 < a1 ) + ( a2 > a1 ) ) == 3 );
    }
    {
        podder<std::uint16_t> a1{ 1, 2, 3, 4, 5 };
        podder<std::uint16_t> a2{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };

        b &= ( ( ( a1 == a2 ) + ( a1 != a2 ) + ( a1 <= a2 ) + ( a1 >= a2 ) + ( a1 < a2 ) + ( a1 > a2 ) ) == 3 );
        b &= ( ( ( a2 == a1 ) + ( a2 != a1 ) + ( a2 <= a1 ) + ( a2 >= a1 ) + ( a2 < a1 ) + ( a2 > a1 ) ) == 3 );
    }
    {
        podder<std::uint16_t> a1{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
        podder<std::uint16_t> a2{ 1, 2, 3, 4 };

        b &= ( ( ( a1 == a2 ) + ( a1 != a2 ) + ( a1 <= a2 ) + ( a1 >= a2 ) + ( a1 < a2 ) + ( a1 > a2 ) ) == 3 );
        b &= ( ( ( a2 == a1 ) + ( a2 != a1 ) + ( a2 <= a1 ) + ( a2 >= a1 ) + ( a2 < a1 ) + ( a2 > a1 ) ) == 3 );
    }
    {
        podder<std::uint16_t> a1{ 1, 2, 3, 4 };
        podder<std::uint16_t> a2{ 1, 2, 3, 4, 5, 6, 7 };

        b &= ( ( ( a1 == a2 ) + ( a1 != a2 ) + ( a1 <= a2 ) + ( a1 >= a2 ) + ( a1 < a2 ) + ( a1 > a2 ) ) == 3 );
        b &= ( ( ( a2 == a1 ) + ( a2 != a1 ) + ( a2 <= a1 ) + ( a2 >= a1 ) + ( a2 < a1 ) + ( a2 > a1 ) ) == 3 );
    }
    {
        podder<std::uint16_t> a1{ 1, 2, 3, 4 };
        podder<std::uint16_t> a2{ 2, 2, 3, 4 };

        b &= ( ( ( a1 == a2 ) + ( a1 != a2 ) + ( a1 <= a2 ) + ( a1 >= a2 ) + ( a1 < a2 ) + ( a1 > a2 ) ) == 3 );
        b &= ( ( ( a2 == a1 ) + ( a2 != a1 ) + ( a2 <= a1 ) + ( a2 >= a1 ) + ( a2 < a1 ) + ( a2 > a1 ) ) == 3 );
    }
    {
        podder<std::uint16_t> a1{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
        podder<std::uint16_t> a2{ 2, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };

        b &= ( ( ( a1 == a2 ) + ( a1 != a2 ) + ( a1 <= a2 ) + ( a1 >= a2 ) + ( a1 < a2 ) + ( a1 > a2 ) ) == 3 );
        b &= ( ( ( a2 == a1 ) + ( a2 != a1 ) + ( a2 <= a1 ) + ( a2 >= a1 ) + ( a2 < a1 ) + ( a2 > a1 ) ) == 3 );
    }
    {
        podder<std::uint16_t> a1{ 1, 2, 3, 4, 5 };
        podder<std::uint16_t> a2{ 2, 2, 3, 4 };

        b &= ( ( ( a1 == a2 ) + ( a1 != a2 ) + ( a1 <= a2 ) + ( a1 >= a2 ) + ( a1 < a2 ) + ( a1 > a2 ) ) == 3 );
        b &= ( ( ( a2 == a1 ) + ( a2 != a1 ) + ( a2 <= a1 ) + ( a2 >= a1 ) + ( a2 < a1 ) + ( a2 > a1 ) ) == 3 );
    }
    {
        podder<std::uint16_t> a1{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
        podder<std::uint16_t> a2{ 2, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };

        b &= ( ( ( a1 == a2 ) + ( a1 != a2 ) + ( a1 <= a2 ) + ( a1 >= a2 ) + ( a1 < a2 ) + ( a1 > a2 ) ) == 3 );
        b &= ( ( ( a2 == a1 ) + ( a2 != a1 ) + ( a2 <= a1 ) + ( a2 >= a1 ) + ( a2 < a1 ) + ( a2 > a1 ) ) == 3 );
    }
    {
        podder<std::uint16_t> a1{ 1, 2, 3, 4, 5 };
        podder<std::uint16_t> a2{ 2, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };

        b &= ( ( ( a1 == a2 ) + ( a1 != a2 ) + ( a1 <= a2 ) + ( a1 >= a2 ) + ( a1 < a2 ) + ( a1 > a2 ) ) == 3 );
        b &= ( ( ( a2 == a1 ) + ( a2 != a1 ) + ( a2 <= a1 ) + ( a2 >= a1 ) + ( a2 < a1 ) + ( a2 > a1 ) ) == 3 );
    }
    {
        podder<std::uint16_t> a1{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
        podder<std::uint16_t> a2{ 2, 2, 3, 4 };

        b &= ( ( ( a1 == a2 ) + ( a1 != a2 ) + ( a1 <= a2 ) + ( a1 >= a2 ) + ( a1 < a2 ) + ( a1 > a2 ) ) == 3 );
        b &= ( ( ( a2 == a1 ) + ( a2 != a1 ) + ( a2 <= a1 ) + ( a2 >= a1 ) + ( a2 < a1 ) + ( a2 > a1 ) ) == 3 );
    }
    {
        podder<std::uint16_t> a1{ 1, 2, 3, 4 };
        podder<std::uint16_t> a2{ 2, 2, 3, 4, 5, 6, 7 };

        b &= ( ( ( a1 == a2 ) + ( a1 != a2 ) + ( a1 <= a2 ) + ( a1 >= a2 ) + ( a1 < a2 ) + ( a1 > a2 ) ) == 3 );
        b &= ( ( ( a2 == a1 ) + ( a2 != a1 ) + ( a2 <= a1 ) + ( a2 >= a1 ) + ( a2 < a1 ) + ( a2 > a1 ) ) == 3 );
    }
    {
        podder<std::uint16_t> a1{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
        podder<std::uint16_t> a2{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 999 };

        b &= ( ( ( a1 == a2 ) + ( a1 != a2 ) + ( a1 <= a2 ) + ( a1 >= a2 ) + ( a1 < a2 ) + ( a1 > a2 ) ) == 3 );
        b &= ( ( ( a2 == a1 ) + ( a2 != a1 ) + ( a2 <= a1 ) + ( a2 >= a1 ) + ( a2 < a1 ) + ( a2 > a1 ) ) == 3 );
    }
    {
        podder<std::uint16_t> a1{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
        podder<std::uint16_t> a2{ 2, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 };

        b &= ( ( ( a1 == a2 ) + ( a1 != a2 ) + ( a1 <= a2 ) + ( a1 >= a2 ) + ( a1 < a2 ) + ( a1 > a2 ) ) == 3 );
        b &= ( ( ( a2 == a1 ) + ( a2 != a1 ) + ( a2 <= a1 ) + ( a2 >= a1 ) + ( a2 < a1 ) + ( a2 > a1 ) ) == 3 );
    }
    {
        podder<std::uint16_t> a1{ 1, 2, 3, 4 };
        std::vector<std::uint16_t> a2{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 };

        b &= ( ( ( a1 == a2 ) + ( a1 != a2 ) + ( a1 <= a2 ) + ( a1 >= a2 ) + ( a1 < a2 ) + ( a1 > a2 ) ) == 3 );
    }
    {
        podder<std::uint16_t> a1{ 1, 2, 3, 4 };
        std::vector<std::uint16_t> a2{ 2, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 };

        b &= ( ( ( a1 == a2 ) + ( a1 != a2 ) + ( a1 <= a2 ) + ( a1 >= a2 ) + ( a1 < a2 ) + ( a1 > a2 ) ) == 3 );
    }
    {
        podder<std::uint16_t> a1{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };
        std::vector<std::uint16_t> a2{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 };

        b &= ( ( ( a1 == a2 ) + ( a1 != a2 ) + ( a1 <= a2 ) + ( a1 >= a2 ) + ( a1 < a2 ) + ( a1 > a2 ) ) == 3 );
    }
    {
        podder<std::uint16_t> a1{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };
        std::vector<std::uint16_t> a2{ 2, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 };

        b &= ( ( ( a1 == a2 ) + ( a1 != a2 ) + ( a1 <= a2 ) + ( a1 >= a2 ) + ( a1 < a2 ) + ( a1 > a2 ) ) == 3 );
    }

    return b;
}

int main ( ) {

    std::cout << "range constructor test " << std::boolalpha << podder_range_constructor_test ( ) << nl;
    std::cout << "emplace_back test " << std::boolalpha << podder_emplace_back_test ( ) << nl;
    std::cout << "insert test " << std::boolalpha << podder_insert_test ( ) << nl;
    std::cout << "comparison test " << std::boolalpha << podder_comparison_test ( ) << nl;

    std::cout << sizeof ( podder<std::uint8_t> ) << nl;
    std::cout << sizeof ( podder<std::uint16_t> ) << nl;
    std::cout << sizeof ( podder<std::uint32_t> ) << nl;
    std::cout << sizeof ( podder<std::uint64_t> ) << nl;

    return EXIT_SUCCESS;
}
