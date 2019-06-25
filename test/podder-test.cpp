
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
#include <fstream>
#include <sax/iostream.hpp> // <iostream> + nl, sp etc. defined...
#include <iterator>
#include <list>
#include <map>
#include <random>
#include <string>
#include <type_traits>
#include <vector>

#ifdef _WIN32
#pragma comment ( lib, "Shlwapi.lib" )
#endif

#if defined ( _DEBUG )
#pragma comment ( lib, "benchmark_maind.lib" )
#pragma comment ( lib, "benchmarkd.lib" )
#else
#pragma comment ( lib, "benchmark_main.lib" )
#pragma comment ( lib, "benchmark.lib" )
#endif

// #define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
// #include <catch.hpp>

#include <sax/splitmix.hpp>
#include <sax/uniform_int_distribution.hpp>

#include "podder.hpp"



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
bool podder_emplace_back_type_test ( std::size_t x ) { // test emplace_back.

    std::vector<container> vv;
    sax::splitmix64 gen ( x );
    auto dis = sax::uniform_int_distribution<std::size_t> ( std::size_t { 0 }, std::min ( static_cast<std::size_t> ( std::numeric_limits<std::uint8_t>::max ( ) - 1 ), static_cast<std::size_t> ( std::numeric_limits<value_type>::max ( ) - 1 ) ) );
    std::uint64_t a = 0;

    for ( std::size_t i = 0; i < 200; ++i ) {
        vv.emplace_back ( );
        const auto n = dis ( gen );
        for ( std::size_t j = 0; j < n; ++j ) {
            vv.back ( ).emplace_back ( ( value_type ) j );
            a += j;
        }
    }

    std::uint64_t b = 0;

    for ( auto v : vv ) {
        b += sum ( v );
    }

    return a == b;
}

bool podder_emplace_back_test_all_types ( std::size_t x ) {

    bool a = true, b = true, c = true, d = true;

    a = podder_emplace_back_type_test<std::uint8_t, podder<std::uint8_t >> ( x );
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

int main ( ) {

    std::cout << podder_emplace_back_test ( ) << nl;

    return EXIT_SUCCESS;
}
