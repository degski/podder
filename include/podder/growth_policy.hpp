
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

#pragma once

#include <cmath>
#include <cstdint>

#include <limits>
#include <ratio>


#ifndef pure_function
#ifdef __clang__
#define pure_function __attribute__ ( ( const ) )
#else
#define pure_function
#endif
#endif


template<typename SizeType = std::size_t>
struct visual_studio_growth_policy {

    using size_type = SizeType;

    // The growth policy in the VC-implementation of std::vector ( c = c + c / 2 ).
    static constexpr size_type grow_capacity_from ( size_type const capacity_ = 1 ) noexcept pure_function {
        assert ( capacity_ < ( std::numeric_limits<size_type>::max ( ) - ( capacity_ / 2 ) ) );
        return std::max ( size_type { 2 }, capacity_ + capacity_ / 2 );
    }
};


template<typename R = std::ratio<3, 2>, typename SizeType = std::size_t>
struct ratio_growth_policy {

    using size_type = SizeType;

    static constexpr size_type grow_capacity_from ( size_type const capacity_ = 1 ) noexcept pure_function {
        // assert tbd
        return std::max ( std::size_t { 2 }, ( R::num * static_cast<std::size_t> ( capacity_ ) + 1 ) / R::den );
    }
};


namespace detail {
// The Golden Ratio, the ratio of the 2 largest consecutive Fibonacci numbers
// representable in std::intmax_t...
//  http://www.bigprimes.net/archive/fibonacci/

using golden_ratio_64 = std::ratio<7540113804746346429, 4660046610375530309>; // 64 bit
using golden_ratio_32 = std::ratio<         1836311903,          1134903170>; // 32 bit
}

using golden_ratio = std::conditional<( sizeof ( std::uint32_t ) == sizeof ( std::uintptr_t ) ), detail::golden_ratio_32, detail::golden_ratio_64>::type;

template<typename SizeType = std::size_t>
struct golden_ratio_growth_policy : public ratio_growth_policy<golden_ratio, SizeType> { };


template<typename SizeType = std::size_t>
struct additive_growth_policy {

    using size_type = SizeType;

    // parameter specifies the requested additional capacity.
    static constexpr size_type grow_capacity_from ( size_type const capacity_ = 0, size_type const added_capacity_ = 1 ) noexcept pure_function {
        return capacity_ + added_capacity_;
    }
};


template<typename GrowthPolicy>
constexpr typename GrowthPolicy::size_type required_capacity ( const typename GrowthPolicy::size_type capacity_, const typename GrowthPolicy::size_type requested_ ) noexcept {
    typename GrowthPolicy::size_type capacity = GrowthPolicy::grow_capacity_from ( capacity_ );
    while ( capacity < requested_ ) {
        capacity = GrowthPolicy::grow_capacity_from ( capacity );
    }
    return capacity;
}

template<typename GrowthPolicy>
constexpr typename GrowthPolicy::size_type required_capacity ( const typename GrowthPolicy::size_type capacity_, const typename GrowthPolicy::size_type requested_, const typename GrowthPolicy::size_type added_capacity_ = 1 ) noexcept {
    typename GrowthPolicy::size_type capacity = GrowthPolicy::grow_capacity_from ( capacity_ );
    while ( capacity < requested_ ) {
        capacity = GrowthPolicy::grow_capacity_from ( capacity );
    }
    return capacity;
}


#ifdef pure_function
#undef pure_function
#endif
