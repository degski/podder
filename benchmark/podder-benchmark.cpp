
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
#include <iterator>
#include <list>
#include <map>
#include <random>
#include <sax/iostream.hpp>
#include <string>
#include <type_traits>
#include <vector>

#include <benchmark/benchmark.h>

#ifdef _WIN32
#    pragma comment( lib, "Shlwapi.lib" )
#if defined( _DEBUG )
#    pragma comment( lib, "benchmark_maind.lib" )
#    pragma comment( lib, "benchmarkd.lib" )
#else
#    pragma comment( lib, "benchmark_main.lib" )
#    pragma comment( lib, "benchmark.lib" )
#endif
#endif

#include <sax/splitmix.hpp>
#include <sax/uniform_int_distribution.hpp>

#include "podder.hpp"

// integer logN.

template<int Base, typename T, typename sfinae = std::enable_if_t<std::conjunction_v<std::is_integral<T>, std::is_unsigned<T>>>>
constexpr T iLog ( const T n_, const T p_ = T ( 0 ) ) noexcept {
    return n_ < Base ? p_ : iLog<Base, T, sfinae> ( n_ / Base, p_ + 1 );
}

// integer log2.

template<typename T, typename = std::enable_if_t<std::conjunction_v<std::is_integral<T>, std::is_unsigned<T>>>>
constexpr T ilog2 ( const T n_ ) noexcept {
    return iLog<2, T> ( n_ );
}

template<typename T, typename = std::enable_if_t<std::conjunction_v<std::is_integral<T>, std::is_unsigned<T>>>>
constexpr T next_power_2 ( const T n_ ) noexcept {
    return n_ > 2 ? T ( 1 ) << ( ilog2<T> ( n_ - 1 ) + 1 ) : n_;
}

template<typename T, typename = std::enable_if_t<std::conjunction_v<std::is_integral<T>, std::is_unsigned<T>>>>
constexpr bool is_power_2 ( const T n_ ) noexcept {
    return n_ and not( n_ & ( n_ - 1 ) );
}

template<typename ValueType>
static void custom_arguments ( benchmark::internal::Benchmark * b ) {
    const std::size_t bs = podder<ValueType>::buff_size ( );
    b->Args ( { ( long long ) bs, 8192 / sizeof ( ValueType ) } );
    for ( std::size_t i = next_power_2 ( bs + is_power_2 ( bs ) ); i <= 8192u / sizeof ( ValueType ); i <<= 1 )
        b->Args ( { ( long long ) i, 8192 / sizeof ( ValueType ) } );
}

template<class Container>
void bm_emplace_back_random ( benchmark::State & state ) noexcept {
    using value_type = typename Container::value_type;
    sax::splitmix64 gen;
    for ( auto _ : state ) {
        state.PauseTiming ( );
        const std::int64_t is = sax::uniform_int_distribution<std::int64_t> ( 0, state.range ( 0 ) - 1 ) ( gen );
        Container data ( static_cast<typename Container::size_type> ( is ) );
        benchmark::DoNotOptimize ( data.data ( ) );
        auto i = sax::uniform_int_distribution<std::int64_t> ( 1, state.range ( 0 ) - is ) ( gen );
        state.ResumeTiming ( );
        while ( i-- )
            data.emplace_back ( static_cast<value_type> ( i ) );
        benchmark::ClobberMemory ( );
    }
}


#include <pector/pector.h>
#include <pector/malloc_allocator.h>
#include <pector/mimalloc_allocator.h>

template<typename T, typename S>
using pector = pt::pector<T, pt::malloc_allocator<T, true, false>, S, pt::default_recommended_size, false>;
template<typename T, typename S>
using mipector = pt::pector<T, pt::mimalloc_allocator<T, true, false>, S, pt::default_recommended_size, false>;


BENCHMARK_TEMPLATE ( bm_emplace_back_random, std::vector<std::uint8_t> )
    ->Apply ( custom_arguments<std::uint8_t> )
    ->Repetitions ( 4 )
    ->ReportAggregatesOnly ( true );
BENCHMARK_TEMPLATE ( bm_emplace_back_random, pector<std::uint8_t, std::int64_t> )
    ->Apply ( custom_arguments<std::uint8_t> )
    ->Repetitions ( 4 )
    ->ReportAggregatesOnly ( true );
BENCHMARK_TEMPLATE ( bm_emplace_back_random, mipector<std::uint8_t, std::int64_t> )
    ->Apply ( custom_arguments<std::uint8_t> )
    ->Repetitions ( 4 )
    ->ReportAggregatesOnly ( true );
/*
BENCHMARK_TEMPLATE ( bm_emplace_back_random, podder<std::uint8_t, std::int64_t> )
    ->Apply ( custom_arguments<std::uint8_t> )
    ->Repetitions ( 4 )
    ->ReportAggregatesOnly ( true );
*/
