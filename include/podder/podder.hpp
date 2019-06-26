
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

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include <algorithm>
#include <array>
#include <sax/iostream.hpp>
#include <memory>
#include <optional>
#include <string>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "growth_policy.hpp"
#include "null_allocator.hpp"
#include "tagged_pointer.hpp"

#ifndef pure_function
#ifdef __clang__
#define pure_function __attribute__ ( ( const ) )
#else
#define pure_function
#endif
#endif

#ifdef small
#define podder_undef_small small
#undef small
#endif

#define PRIVATE
#define PUBLIC public:

namespace detail { // keep the macros out of the code (as it's ugly).
#ifdef _DEBUG
using is_debug = std::true_type;
using is_release = std::false_type;
#else
using is_debug = std::false_type;
using is_release = std::true_type;
#endif
} // namespace detail

template<int I> void div ( char ( * ) [ I % 2 == 0 ] = 0 ) {
    /* this is taken when I is even */
}

template<int I> void div ( char ( * ) [ I % 2 == 1 ] = 0 ) {
    /* this is taken when I is odd */
}

#define USE_MIMALLOC true

#if USE_MIMALLOC
    #include <mimalloc.h>
#endif

#if _WIN32 and USE_MIMALLOC
    #if defined ( USE_THIN_LTO )
        #pragma comment ( lib, "mimalloc-thin-lto.lib" )
    #else
        #pragma comment ( lib, "mimalloc.lib" )
    #endif
#endif

namespace pdr {
#if USE_MIMALLOC
[[ nodiscard ]] inline void* malloc ( std::size_t size ) noexcept {
    return mi_malloc ( size );
}
[[ nodiscard ]] inline void* zalloc ( std::size_t size ) noexcept {
    return mi_zalloc ( size );
}
[[ nodiscard ]] inline void* calloc ( std::size_t num, std::size_t size ) noexcept {
    return mi_calloc ( num, size );
}
[[ nodiscard ]] inline void* realloc ( void* ptr, std::size_t new_size ) noexcept {
    return mi_realloc ( ptr, new_size );
}
inline void free ( void* ptr ) noexcept {
    mi_free ( ptr );
}
#else
[[ nodiscard ]] inline void* malloc ( std::size_t size ) noexcept {
    return std::malloc ( size );
}
[[ nodiscard ]] inline void* zalloc ( std::size_t size ) noexcept {
    void * const p = std::malloc ( size );
    std::memset ( p, 0, size );
    return p;
}
[[ nodiscard ]] inline void* calloc ( std::size_t num, std::size_t size ) noexcept {
    return std::calloc ( num, size );
}
[[ nodiscard ]] inline void* realloc ( void* ptr, std::size_t new_size ) noexcept {
    return std::realloc ( ptr, new_size );
}
inline void free ( void* ptr ) noexcept {
    std::free ( ptr );
}
#endif
} // namespace pdr

template<typename Type = std::uint8_t, typename SizeType = std::size_t, typename GrowthPolicy = visual_studio_growth_policy<SizeType>>
class podder {

    template<typename ST>
    using is_size_type = std::disjunction<std::is_same<ST, std::uint32_t>, std::is_same<SizeType, std::uint64_t>>;

    static_assert ( std::is_trivially_copyable<Type>::value, "Type must be trivially copyable!" );
    static_assert ( is_size_type<SizeType>::value, "SizeType must be an unsigned 32- or 64-bit integer type!" );

    PUBLIC

    using value_type = Type;
    using optional_value_type = std::optional<value_type>;
    using pointer = value_type * ;
    using const_pointer = value_type const *;

    using reference = value_type & ;
    using const_reference = value_type const &;
    using rv_reference = value_type && ;

    using size_type = SizeType;
    using signed_size_type = typename std::conditional<std::is_same<size_type, std::uint32_t>::value, std::int32_t, std::int64_t>::type;
    using difference_type = signed_size_type;

    using allocator_type = ::pdr::detail::null_allocator<Type>;

    using iterator = pointer;
    using const_iterator = const_pointer;
    using reverse_iterator = pointer;
    using const_reverse_iterator = const_pointer;

    PRIVATE

    using is_debug = detail::is_debug;
    using is_release = detail::is_release;
    using growth_policy = GrowthPolicy;

    enum class svo_type : std::uint8_t { medium, small };

    template<typename>
    struct is_std_array : std::false_type { };

    template<typename Item, std::size_t N>
    struct is_std_array<std::array<Item, N>> : std::true_type { };

    template<typename Container>
    using is_contiguous_container =
        std::disjunction<
        std::is_same<Container, podder>,
        std::is_same<Container, std::vector<value_type>>,
        is_std_array<Container>,
        std::is_same<Container, std::initializer_list<value_type>>
        >;

    struct cc_tag { };
    struct dc_tag { };

    template<typename Container>
    using contiguous_container_t =
        std::enable_if_t<is_contiguous_container<Container>::value, cc_tag>;
    template<typename Container>
    using discontiguous_container_t =
        std::enable_if_t<std::negation<is_contiguous_container<Container>>::value, dc_tag>;

    // is_array_iterator.

    template<typename It, typename T, std::size_t S>
    struct is_array_iterator_impl : is_array_iterator_impl<It, T, S - 1> { };
    template<typename It, typename T>
    struct is_array_iterator_impl<It, T, 0> {
        static constexpr bool value = false;
        using type = std::false_type;
    };
    template<typename T, std::size_t S>
    struct is_array_iterator_impl<typename std::array<T, S>::iterator, T, S> {
        static constexpr bool value = true;
    };
    template<typename T, std::size_t S>
    struct is_array_iterator_impl<typename std::array<T, S>::const_iterator, T, S> {
        static constexpr bool value = true;
    };
    template<typename It>
    struct is_array_iterator {
        // 1012 is the max value untill the maximum recursive levels of 1024 get exceeded (of my clang
        // implementation), decrease or increase this value as required/allowed by your implementation.
        static constexpr bool value = is_array_iterator_impl<It, typename It::value_type, 384>::value;
    };

    // is_contiguous_input_iterator.

    template<typename It>
    using is_contiguous_input_iterator =
        std::disjunction<
        std::is_same<It, typename std::vector<typename It::value_type>::iterator>,
        std::is_same<It, typename std::vector<typename It::value_type>::const_iterator>,
        is_array_iterator<It>,
        std::is_same<It, iterator>,
        std::is_same<It, const_iterator>
        >;
    template<typename It>
    using is_discontiguous_input_iterator =
        std::conjunction<
        std::is_base_of<std::input_iterator_tag, typename std::iterator_traits<It>::iterator_category>,
        std::negation<is_contiguous_input_iterator<It>>
        >;

    struct cii_tag { };
    struct dii_tag { };

    template<typename It>
    using contiguous_input_iterator_t =
        std::enable_if_t<is_contiguous_input_iterator<It>::value, cii_tag>;
    template<typename It>
    using discontiguous_input_iterator_t =
        std::enable_if_t<is_discontiguous_input_iterator<It>::value, dii_tag>;

    PUBLIC

    // constructors.

    explicit podder ( ) noexcept { // (explicit, LWG 2193) default construction, either small or no_svo, depending on value_type.
        if constexpr ( svo ( ) ) {
            small_clear ( ); // zeroes whole structure in debug, sets the is_small flag to true.
        }
        else {
            // zeroes the class. end is nullptr, capacity = 0, or when checking
            // for realocation: size == capacity, evaluates to true, i.e. needs
            // (re-)alloc'ing.
            std::memset ( ( void* ) this, 0, sizeof ( *this ) );
        }
    }
    podder ( size_type count, const_reference value, signed_size_type stride = 0 ) {
        value_type v = value;
        if ( count ) {
            if constexpr ( svo ( ) ) {
                if ( count <= buff_size ( ) ) {
                    set_small_size ( count );
                    pointer p = d.s.buffer;
                    while ( count-- ) {
                        new ( p++ ) value_type ( std::forward<value_type> ( { v } ) );
                        v += static_cast<value_type> ( stride );
                    }
                }
                else {
                    new ( & d.m ) medium ( std::forward<medium> ( { count, count, static_cast<pointer> ( pdr::malloc ( count * sizeof ( value_type ) ) ) } ) );
                    while ( count-- ) {
                        new ( d.m.end++ ) value_type ( std::forward<value_type> ( { v } ) );
                        v += static_cast<value_type> ( stride );
                    }
                }
            }
            else {
                new ( & d.m ) medium ( std::forward<medium> ( { count, count, static_cast<pointer> ( pdr::malloc ( count * sizeof ( value_type ) ) ) } ) );
                while ( count-- ) {
                    new ( d.m.end++ ) value_type ( std::forward<value_type> ( { v } ) );
                    v += static_cast<value_type> ( stride );
                }
            }
        }
        else {
            small_clear ( );
        }
    }
    podder ( size_type count ) {
        if ( count ) {
            if constexpr ( svo ( ) ) {
                if ( count <= buff_size ( ) ) {
                    set_small_size ( count );
                    pointer p = d.s.buffer;
                    while ( count-- )
                        new ( p++ ) value_type ( );
                }
                else {
                    new ( & d.m ) medium ( std::forward<medium> ( { count, count, static_cast<pointer> ( pdr::malloc ( count * sizeof ( value_type ) ) ) } ) );
                    while ( count-- )
                        new ( d.m.end++ ) value_type ( );
                }
            }
            else {
                new ( & d.m ) medium ( std::forward<medium> ( { count, count, static_cast<pointer> ( pdr::malloc ( count * sizeof ( value_type ) ) ) } ) );
                while ( count-- )
                    new ( d.m.end++ ) value_type ( );
            }
        }
        else {
            small_clear ( );
        }
    }
    podder ( int count ) :
        podder ( static_cast<size_type> ( count ) ) {
    }
    template<typename InputIt, contiguous_input_iterator_t<InputIt> * = nullptr>
    podder ( InputIt first, InputIt last ) :
        podder ( &*first, static_cast<size_type> ( last - first ) ) { }
    template<typename InputIt, discontiguous_input_iterator_t<InputIt> * = nullptr>
    podder ( InputIt first, InputIt last ) {
        assert ( std::distance ( first, last ) >= 0 );
        size_type const count = ( size_type ) std::distance ( first, last );
        if ( count ) {
            if constexpr ( svo ( ) ) {
                if ( count <= buff_size ( ) ) {
                    set_small_size ( count );
                    iterator end ( d.s.buffer );
                    while ( first != last )
                        *end++ = *first++;
                }
                else {
                    new ( & d.m ) medium ( std::forward<medium> ( { count, count, static_cast<pointer> ( pdr::malloc ( count * sizeof ( value_type ) ) ) } ) );
                    while ( first != last )
                        *d.m.end++ = *first++;
                }
            }
            else { // constexpr else.
                new ( & d.m ) medium ( std::forward<medium> ( { count, count, static_cast<pointer> ( pdr::malloc ( count * sizeof ( value_type ) ) ) } ) );
                while ( first != last )
                    *d.m.end++ = *first++;
            }
        }
        else {
            small_clear ( );
        }
    }
    podder ( podder const & p ) noexcept {
        if ( p.d.s.is_small ) {
            std::memcpy ( ( void* ) this, ( void* ) & p, sizeof ( podder ) );
        }
        else {
            new ( & d.m ) medium { p.d.m.size, p.d.m.capacity, static_cast<pointer> ( pdr::malloc ( d.m.capacity * sizeof ( value_type ) ) ) };
            std::memcpy ( ( void* ) d.m.end, ( void* ) p.begin_pointer ( ), d.m.size * sizeof ( value_type ) );
            d.m.end += d.m.size;
        }
    }
    podder ( podder && p ) noexcept {
        std::memcpy ( ( void* ) this, ( void* ) & p, sizeof ( podder ) );
        p.small_clear ( );
    }
    podder ( const std::initializer_list<value_type> il ) :
        podder ( il.begin ( ), static_cast< size_type > ( il.size ( ) ) ) { }
    template<auto S>
    podder ( std::array<value_type, S> const & a ) :
        podder ( a.d ( ), static_cast< size_type > ( S ) ) { }
    podder ( std::vector<value_type> const & v ) :
        podder ( v.d ( ), static_cast< size_type > ( v.size ( ) ) ) { }
    template<auto S>
    podder ( value_type const ( *a ) [ S ] ) :
        podder ( a, static_cast< size_type > ( S ) ) { }
    template<typename Container>
    podder ( Container const & c ) :
        podder ( std::begin ( c ), std::end ( c ) ) { }
    explicit podder ( const_pointer first, const_pointer last ) :
        podder ( first, static_cast< size_type > ( last - first ) ) { }
    explicit podder ( const_pointer first, size_type const count ) {
        assert ( count >= 0 );
        if ( count ) {
            if constexpr ( svo ( ) ) {
                if ( count <= buff_size ( ) ) {
                    set_small_size ( count );
                    std::memcpy ( ( void* ) d.s.buffer, ( void* ) first, count * sizeof ( value_type ) );
                }
                else {
                    new ( & d.m ) medium ( std::forward<medium> ( { count, count, static_cast<pointer> ( pdr::malloc ( count * sizeof ( value_type ) ) ) } ) );
                    std::memcpy ( ( void* ) d.m.end, ( void* ) first, count * sizeof ( value_type ) );
                    d.m.end += d.m.size;
                }
            }
            else {
                new ( & d.m ) medium ( std::forward<medium> ( { count, count, static_cast<pointer> ( pdr::malloc ( count * sizeof ( value_type ) ) ) } ) );
                std::memcpy ( ( void* ) d.m.end, ( void* ) first, count * sizeof ( value_type ) );
                d.m.end += d.m.size;
            }
        }
        else {
            small_clear ( );
        }
    }

    // destructor.

    ~podder ( ) noexcept {
        if ( not d.s.is_small ) {
            pdr::free ( d.m.end - d.m.size );
            return;
        }
        if constexpr ( is_debug::value ) { // checking whether the above condition is sufficiently strong.
            if ( not d.m.end ) {
                std::cerr << "This code in the destructor should be free-ing as well." << nl;
            }
            return;
        }
    }

    // assignment operator.

    [[ maybe_unused ]] podder & operator = ( podder const & rhs ) {
        size_type const count = rhs.size ( );
        if ( not count ) {
            clear ( );
        }
        else if ( count <= buff_size ( ) ) {
            if ( not d.s.is_small )
                pdr::free ( d.m.end - d.m.size );
            std::memcpy ( ( void* ) this, ( void* ) & rhs, sizeof ( podder ) );
        }
        else {
            if ( d.s.is_small ) {
                new ( & d.m ) medium ( std::forward<medium> ( { count, count, static_cast<pointer> ( pdr::malloc ( count * sizeof ( value_type ) ) ) } ) );
                std::memcpy ( ( void* ) d.m.end, ( void* ) ( rhs.d.m.end - rhs.d.m.size ), count * sizeof ( value_type ) );
                d.m.end += count;
            }
            else {
                pointer const b = d.m.end - d.m.size;
                if ( count <= d.m.capacity ) {
                    std::memcpy ( ( void* ) b, ( void* ) ( rhs.d.m.end - rhs.d.m.size ), count * sizeof ( value_type ) );
                    d.m.size = count;
                    d.m.end = b + count;
                }
                else {
                    d.m.capacity = d.m.size = count;
                    d.m.end = static_cast<pointer> ( pdr::realloc ( ( void* ) b, count * sizeof ( value_type ) ) );
                    std::memcpy ( ( void* ) d.m.end, ( void* ) ( rhs.d.m.end - rhs.d.m.size ), count * sizeof ( value_type ) );
                    d.m.end += count;
                }
            }
        }
        return *this;
    }
    template<typename Container>
    [[ maybe_unused ]] podder & operator = ( Container const & container ) {
        size_type const count = static_cast< size_type > ( container.size ( ) );
        if ( not count ) {
            clear ( );
            return *this;
        }
        if constexpr ( is_contiguous_container<Container>::value ) {
            if ( count <= buff_size ( ) ) {
                if ( not d.s.is_small )
                    pdr::free ( d.m.end - d.m.size );
                std::memcpy ( ( void* ) d.s.buffer, ( void* ) &*std::begin ( container ), count * sizeof ( value_type ) );
                set_small_size ( count );
            }
            else {
                if ( d.s.is_small ) {
                    new ( & d.m ) medium ( std::forward<medium> ( { count, count, static_cast<pointer> ( pdr::malloc ( count * sizeof ( value_type ) ) ) } ) );
                    std::memcpy ( ( void* ) d.m.end, ( void* ) &*std::begin ( container ), count * sizeof ( value_type ) );
                    d.m.end += count;
                }
                else {
                    pointer const p = d.m.end - d.m.size;
                    if ( count <= d.m.capacity ) {
                        std::memcpy ( ( void* ) p, ( void* ) &*std::begin ( container ), count * sizeof ( value_type ) );
                        d.m.size = count;
                        d.m.end = p + count;
                    }
                    else {
                        d.m.capacity = d.m.size = count;
                        d.m.end = static_cast<pointer> ( pdr::realloc ( p, count * sizeof ( value_type ) ) );
                        std::memcpy ( ( void* ) d.m.end, ( void* ) &*std::begin ( container ), count * sizeof ( value_type ) );
                        d.m.end += count;
                    }
                }
            }
        }
        else { // discontiguous container copy assignment.
            auto first = std::cbegin ( container );
            auto const last = std::cend ( container );
            if ( count <= buff_size ( ) ) {
                if ( not d.s.is_small )
                    pdr::free ( d.m.end - d.m.size );
                set_small_size ( count );
                iterator it ( d.s.buffer );
                while ( first != last )
                    *it++ = *first++;
            }
            else {
                if ( d.s.is_small ) {
                    new ( & d.m ) medium ( std::forward<medium> ( { count, count, static_cast<pointer> ( pdr::malloc ( count * sizeof ( value_type ) ) ) } ) );
                    while ( first != last )
                        *d.m.end++ = *first++;
                }
                else {
                    pointer p = d.m.end - d.m.size;
                    if ( count <= d.m.capacity ) {
                        d.m.size = count;
                        while ( first != last )
                            *p++ = *first++;
                        d.m.end = p;
                    }
                    else {
                        d.m.capacity = d.m.size = count;
                        d.m.end = static_cast<pointer> ( pdr::realloc ( p, count * sizeof ( value_type ) ) );
                        while ( first != last )
                            *d.m.end++ = *first++;
                    }
                }
            }
        }
        return *this;
    }
    template<auto S>
    [[ maybe_unused ]] podder & operator = ( value_type ( *a ) [ S ] ) {
        constexpr size_type count = static_cast< size_type > ( S );
        if constexpr ( count <= buff_size ( ) ) {
            if ( not d.s.is_small )
                pdr::free ( d.m.end - d.m.size );
            std::memcpy ( ( void* ) d.s.buffer, ( void* ) a, count * sizeof ( value_type ) );
            set_small_size ( count );
        }
        else {
            if ( d.s.is_small ) {
                new ( & d.m ) medium ( std::forward<medium> ( { count, count, static_cast<pointer> ( pdr::malloc ( count * sizeof ( value_type ) ) ) } ) );
                std::memcpy ( ( void* ) d.m.end, ( void* ) a, count * sizeof ( value_type ) );
                d.m.end += count;
            }
            else {
                pointer const p = d.m.end - d.m.size;
                if ( count <= d.m.capacity ) {
                    std::memcpy ( ( void* ) p, ( void* ) a, count * sizeof ( value_type ) );
                    d.m.size = count;
                    d.m.end = p + count;
                }
                else {
                    d.m.capacity = d.m.size = count;
                    d.m.end = static_cast<pointer> ( pdr::realloc ( p, count * sizeof ( value_type ) ) );
                    std::memcpy ( ( void* ) d.m.end, ( void* ) a, count * sizeof ( value_type ) );
                    d.m.end += count;
                }
            }
        }
    }
    [[ maybe_unused ]] podder & operator = ( podder && rhs ) noexcept {
        if ( not d.s.is_small )
            pdr::free ( d.m.end - d.m.size );
        std::memcpy ( ( void* ) this, ( void* ) & rhs, sizeof ( podder ) );
        rhs.small_clear ( );
        return *this;
    }

    // assign.

    void assign ( size_type count, const_reference value ) {
        assert ( count >= 0 );
        if ( count ) {
            if constexpr ( svo ( ) ) {
                if ( d.s.is_small ) {
                    if ( count <= buff_size ( ) ) {
                        pointer p = d.s.buffer, e = p + count;
                        while ( p != e )
                            *p++ = value;
                        d.s.size = count;
                    }
                    else {
                        new ( & d.m ) medium ( std::forward<medium> ( { count, count, static_cast<pointer> ( pdr::malloc ( count * sizeof ( value_type ) ) ) } ) );
                        pointer e = d.m.end + count;
                        while ( d.m.end != e )
                            *d.m.end++ = value;
                    }
                }
                else {
                    if ( count > d.m.capacity ) {
                        pdr::free ( d.m.end - d.m.size );
                        new ( & d.m ) medium ( std::forward<medium> ( { count, count, static_cast<pointer> ( pdr::malloc ( count * sizeof ( value_type ) ) ) } ) );
                        pointer e = d.m.end + count;
                        while ( d.m.end != e )
                            *d.m.end++ = value;
                    }
                    else {
                        pointer p = d.m.end - d.m.size, e = p + count;
                        while ( p != e )
                            *p++ = value;
                        d.m.end += count - d.m.size;
                        d.m.size = count;
                    }
                }
            }
            else {
                if ( count > d.m.capacity ) {
                    pdr::free ( d.m.end - d.m.size );
                    new ( & d.m ) medium ( std::forward<medium> ( { count, count, static_cast<pointer> ( pdr::malloc ( count * sizeof ( value_type ) ) ) } ) );
                    pointer e = d.m.end + count;
                    while ( d.m.end != e )
                        *d.m.end++ = value;
                }
                else {
                    pointer p = d.m.end - d.m.size, e = p + count;
                    while ( p != e )
                        *p++ = value;
                    d.m.end += count - d.m.size;
                    d.m.size = count;
                }
            }
        }
        else {
            clear ( );
        }
    }
    template<typename InputIt, contiguous_input_iterator_t<InputIt> * = nullptr>
    void assign ( InputIt first, InputIt last ) {
        assign ( &*first, static_cast< size_type > ( last - first ) );
    }
    template<typename InputIt, discontiguous_input_iterator_t<InputIt> * = nullptr>
    void assign ( InputIt first, InputIt last ) {
        size_type const count = static_cast<size_type> ( std::distance ( first, last ) );
        assert ( std::distance ( first, last ) >= 0 );
        if ( count ) {
            if constexpr ( svo ( ) ) {
                if ( d.s.is_small ) {
                    if ( count <= buff_size ( ) ) {
                        pointer p = d.s.buffer;
                        while ( first != last )
                            *p++ = *first++;
                        d.s.size = count;
                    }
                    else {
                        new ( & d.m ) medium ( std::forward<medium> ( { count, count, static_cast<pointer> ( pdr::malloc ( count * sizeof ( value_type ) ) ) } ) );
                        while ( first != last )
                            *d.m.end++ = *first++;
                    }
                }
                else {
                    if ( count > d.m.capacity ) {
                        pdr::free ( d.m.end - d.m.size );
                        new ( & d.m ) medium ( std::forward<medium> ( { count, count, static_cast<pointer> ( pdr::malloc ( count * sizeof ( value_type ) ) ) } ) );
                        while ( first != last )
                            *d.m.end++ = *first++;
                    }
                    else {
                        pointer p = d.m.end - d.m.size;
                        while ( first != last )
                            *p++ = *first++;
                        d.m.end += count - d.m.size;
                        d.m.size = count;
                    }
                }
            }
            else {
                if ( count > d.m.capacity ) {
                    pdr::free ( d.m.end - d.m.size );
                    new ( & d.m ) medium ( std::forward<medium> ( { count, count, static_cast<pointer> ( pdr::malloc ( count * sizeof ( value_type ) ) ) } ) );
                    pointer e = d.m.end + count;
                    while ( d.m.end != e )
                        *d.m.end++ = *first++;
                }
                else {
                    pointer p = d.m.end - d.m.size, e = p + count;
                    while ( p != e )
                        *p++ = *first++;
                    d.m.end += count - d.m.size;
                    d.m.size = count;
                }
            }
        }
        else {
            clear ( );
        }
    }
    void assign ( std::initializer_list<value_type> il ) {
        assign ( il.begin ( ), static_cast<size_type> ( il.size ( ) ) );
    }
    void assign ( const_pointer first, size_type const count ) {
        assert ( count >= 0 );
        if ( count ) {
            if constexpr ( svo ( ) ) {
                if ( d.s.is_small ) {
                    if ( count <= buff_size ( ) ) {
                        std::memcpy ( ( void* ) d.s.buffer, ( void* ) first, count * sizeof ( value_type ) );
                        d.s.size = count;
                    }
                    else {
                        new ( & d.m ) medium ( std::forward<medium> ( { count, count, static_cast<pointer> ( pdr::malloc ( count * sizeof ( value_type ) ) ) } ) );
                        std::memcpy ( ( void* ) d.m.end, ( void* ) first, count * sizeof ( value_type ) );
                        d.m.end += count;
                    }
                }
                else {
                    if ( count > d.m.capacity ) {
                        pdr::free ( d.m.end - d.m.size );
                        new ( & d.m ) medium ( std::forward<medium> ( { count, count, static_cast<pointer> ( pdr::malloc ( count * sizeof ( value_type ) ) ) } ) );
                        std::memcpy ( ( void* ) d.m.end, ( void* ) first, count * sizeof ( value_type ) );
                        d.m.end += count;
                    }
                    else {
                        d.m.end -= d.m.size;
                        std::memcpy ( ( void* ) d.m.end, ( void* ) first, count * sizeof ( value_type ) );
                        d.m.size = count;
                        d.m.end += count;
                    }
                }
            }
            else {
                if ( count > d.m.capacity ) {
                    pdr::free ( d.m.end - d.m.size );
                    new ( & d.m ) medium ( std::forward<medium> ( { count, count, static_cast<pointer> ( pdr::malloc ( count * sizeof ( value_type ) ) ) } ) );
                    std::memcpy ( ( void* ) d.m.end, ( void* ) first, count * sizeof ( value_type ) );
                    d.m.end += count;
                }
                else {
                    d.m.end -= d.m.size;
                    std::memcpy ( ( void* ) d.m.end, ( void* ) first, count * sizeof ( value_type ) );
                    d.m.size = count;
                    d.m.end += count;
                }
            }
        }
        else {
            clear ( );
        }
    }

    // get_allocator.

    allocator_type get_allocator ( ) const noexcept {
        return allocator_type { };
    }

    // at.

    PRIVATE

    reference at_impl ( size_type pos ) {
        pointer p = nullptr;
        if constexpr ( svo ( ) ) {
            if ( d.s.is_small ) {
                if ( pos >= d.s.size )
                    throw std::out_of_range ( std::string ( "index out of bounds, pos = " ) + std::to_string ( pos ) + std::string ( " , size = " ) + std::to_string ( static_cast<size_type> ( d.s.size ) ) + std::string ( "." ) );
                p = d.s.buffer + pos;
            }
            else { // medium.
                if ( pos >= d.m.size )
                    throw std::out_of_range ( std::string ( "index out of bounds, pos = " ) + std::to_string ( pos ) + std::string ( " , size = " ) + std::to_string ( d.m.size ) + std::string ( "." ) );
                p = ( d.m.end - d.m.size ) + pos;
            }
        }
        else {
            if ( pos >= d.m.size )
                throw std::out_of_range ( std::string ( "index out of bounds, pos = " ) + std::to_string ( pos ) + std::string ( " , size = " ) + std::to_string ( d.m.size ) + std::string ( "." ) );
            p = ( d.m.end - d.m.size ) + pos;
        }
        return *p;
    }

    reference at_operator_impl ( size_type pos ) {
        if constexpr ( svo ( ) ) {
            return ( d.s.is_small ? d.s.buffer : ( d.m.end - d.m.size ) ) [ pos ];
        }
        else {
            return ( d.m.end - d.m.size ) [ pos ];
        }
    }

    PUBLIC

    [[ nodiscard ]] reference at ( size_type pos ) {
        return at_impl ( pos );
    }
    [[ nodiscard ]] const_reference at ( size_type pos ) const {
        return at_impl ( pos );
    }

    // operator []

    [[ nodiscard ]] reference operator [ ] ( size_type idx ) noexcept pure_function {
        return at_operator_impl ( idx );
    }
    [[ nodiscard ]] const_reference operator [ ] ( size_type idx ) const noexcept pure_function {
        return at_operator_impl ( idx );
    }

    // front.

    [[ nodiscard ]] reference front ( ) noexcept {
        return *begin_pointer ( );
    }
    [[ nodiscard ]] const_reference front ( ) const noexcept {
        return *begin_pointer ( );
    }

    // back.

    [[ nodiscard ]] reference back ( ) noexcept {
        return *back_pointer ( );
    }
    [[ nodiscard ]] const_reference  back ( ) const noexcept {
        return *back_pointer ( );
    }

    // d.

    [[ nodiscard ]] pointer data ( ) noexcept {
        return begin_pointer ( );
    }
    [[ nodiscard ]] const_pointer data ( ) const noexcept {
        return begin_pointer ( );
    }

    // iterators.

    [[ nodiscard ]] iterator begin ( ) noexcept {
        return iterator { begin_pointer ( ) };
    }
    [[ nodiscard ]] const_iterator begin ( ) const noexcept {
        return const_iterator { begin_pointer ( ) };
    }
    [[ nodiscard ]] const_iterator cbegin ( ) const noexcept {
        return const_iterator { begin_pointer ( ) };
    }

    [[ nodiscard ]] iterator end ( ) noexcept {
        return iterator { end_pointer ( ) };
    }
    [[ nodiscard ]] const_iterator end ( ) const noexcept {
        return const_iterator { end_pointer ( ) };
    }
    [[ nodiscard ]] const_iterator cend ( ) const noexcept {
        return const_iterator { end_pointer ( ) };
    }

    [[ nodiscard ]] reverse_iterator rbegin ( ) noexcept {
        return reverse_iterator { back_pointer ( ) };
    }
    [[ nodiscard ]] const_reverse_iterator rbegin ( ) const noexcept {
        return const_reverse_iterator { back_pointer ( ) };
    }
    [[ nodiscard ]] const_reverse_iterator crbegin ( ) const noexcept {
        return const_reverse_iterator { back_pointer ( ) };
    }

    [[ nodiscard ]] reverse_iterator rend ( ) noexcept {
        return reverse_iterator { begin_pointer ( ) - 1 };
    }
    [[ nodiscard ]] const_reverse_iterator rend ( ) const noexcept {
        return const_reverse_iterator { begin_pointer ( ) - 1 };
    }
    [[ nodiscard ]] const_reverse_iterator crend ( ) const noexcept {
        return const_reverse_iterator { begin_pointer ( ) - 1 };
    }

    // empty.

    [[ nodiscard ]] bool empty ( ) const noexcept {
        return not size ( );
    }

    // full.

    // returns true, iff size == capacity.
    [[ nodiscard ]] bool full ( ) const noexcept {
        if constexpr ( svo ( ) ) {
            return d.s.is_small ? d.s.size == buff_size ( ) : d.m.size == d.m.capacity;
        }
        else {
            return d.m.size == d.m.capacity;
        }
    }

    // size.

    [[ nodiscard ]] size_type size ( ) const noexcept {
        if constexpr ( svo ( ) ) {
            return d.s.is_small ? d.s.size : d.m.size;
        }
        else {
            return d.m.size;
        }
    }

    // maximum size.

    static size_type max_size ( ) noexcept {
        return std::numeric_limits<size_type>::max ( ) - 1;
    }

    // reserve.

    void reserve ( size_type const count ) {
        if constexpr ( svo ( ) ) {
            if ( d.s.is_small ) {
                if ( count > buff_size ( ) ) {
                    pointer const p = static_cast<pointer> ( pdr::malloc ( count * sizeof ( value_type ) ) );
                    std::memcpy ( ( void* ) p, ( void* ) d.s.buffer, d.s.size * sizeof ( value_type ) );
                    d.m.size = static_cast<size_type> ( d.s.size );
                    d.m.capacity = count;
                    d.m.end = p + d.m.size;
                }
            }
            else {
                if ( count > d.m.capacity ) { // relocate.
                    d.m.end = static_cast<pointer> ( pdr::realloc ( d.m.end - d.m.size, ( d.m.capacity = count ) * sizeof ( value_type ) ) ) + d.m.size;
                }
            }
        }
        else {
            if ( d.m.capacity ) { // allocated.
                if ( count > d.m.capacity ) { // relocate.
                    d.m.end = static_cast< pointer > ( pdr::realloc ( d.m.end - d.m.size, ( d.m.capacity = count ) * sizeof ( value_type ) ) ) + d.m.size;
                }
            }
            else { // not allocated, allocate.
                d.m.size = count;
                d.m.capacity = count;
                d.m.end = static_cast<pointer> ( pdr::malloc ( d.m.capacity * sizeof ( value_type ) ) ) + d.m.size;
            }
        }
    }

    // capacity.

    [[ nodiscard ]] size_type capacity ( ) const noexcept {
        if constexpr ( svo ( ) ) {
            return d.s.is_small ? buff_size ( ) : d.m.capacity;
        }
        else {
            return d.m.capacity;
        }
    }

    // shrink to fit.

    void skrink_to_fit ( ) {
        if constexpr ( svo ( ) ) {
            if ( not d.s.is_small )
                if ( d.m.size < d.m.capacity )
                    d.m.end = static_cast<pointer> ( pdr::realloc ( d.m.end - d.m.size, d.m.size * sizeof ( value_type ) ) ) + d.m.size;
        }
        if ( d.m.size < d.m.capacity )
            d.m.end = static_cast<pointer> ( pdr::realloc ( d.m.end - d.m.size, d.m.size * sizeof ( value_type ) ) ) + d.m.size;
    }

    // clear.

    void clear ( ) noexcept {
        if constexpr ( svo ( ) ) {
            if ( d.s.is_small )
                small_clear ( );
            else
                medium_clear ( );
        }
        else {
            if ( d.m.capacity )
                medium_clear ( );
        }
    }

    // insert

    [[ maybe_unused ]] iterator insert ( const_iterator pos, const_reference value ) {
        // std::cout << "iterator insert ( const_iterator pos, const_reference value )" << nl;
        return emplace ( pos, value_type { value } );
    }
    [[ maybe_unused ]] iterator insert ( const_iterator pos, rv_reference value ) {
        // std::cout << "iterator insert ( const_iterator pos, rv_reference value )" << nl;
        return emplace ( pos, std::forward<value_type> ( value ) );
    }
    iterator insert ( const_iterator pos, size_type count, const_reference value ) {
        // std::cout << "iterator insert ( const_iterator pos, size_type count, const_reference value )" << nl;
        if ( count ) {
            if constexpr ( svo ( ) ) {
                if ( d.s.is_small ) {
                    size_type const s1 = d.s.size + count; // s1 = new size.
                    if ( s1 <= buff_size ( ) ) {
                        pointer p = const_cast<pointer> ( pos );
                        pointer const e { p + count };
                        std::memmove ( ( void* ) e, ( void* ) p, reinterpret_cast<char*> ( d.s.buffer + d.s.size ) - reinterpret_cast<char*> ( p ) );
                        d.s.size += count;
                        while ( p != e )
                            *p++ = value;
                    }
                    else {
                        podder const old ( *this );
                        size_type const c = static_cast<size_type> ( growth_policy::grow_capacity_from ( s1 ) );
                        new ( &d.m ) medium ( std::forward<medium> ( { s1, c, static_cast<pointer> ( pdr::malloc ( c * sizeof ( value_type ) ) ) } ) );
                        size_type const i0 = static_cast< size_type > ( pos - d.s.buffer );
                        std::memcpy ( ( void* ) d.m.end, ( void* ) old.d.s.buffer, i0 * sizeof ( value_type ) );
                        pos = ( d.m.end += i0 );
                        const_pointer const e { d.m.end + count };
                        while ( d.m.end != e )
                            *d.m.end++ = value;
                        size_type const i1 = old.d.s.size - i0;
                        std::memcpy ( ( void* ) d.m.end, ( void* ) ( old.d.s.buffer + i0 ), i1 * sizeof ( value_type ) );
                        d.m.end += i1;
                    }
                }
                else { // medium.
                    size_type const s1 = d.m.size + count; // s1 = new size.
                    if ( d.m.size == d.m.capacity ) { // relocation.
                        pointer const old = d.m.end - d.m.size; // begin.
                        d.m.capacity = static_cast<size_type> ( growth_policy::grow_capacity_from ( s1 ) );
                        d.m.end = static_cast<pointer> ( pdr::malloc ( d.m.capacity * sizeof ( value_type ) ) );
                        size_type const i0 = pos - old, i1 = d.m.size - i0;
                        std::memcpy ( ( void* ) d.m.end, ( void* ) old, i0 * sizeof ( value_type ) );
                        pos = ( d.m.end += i0 );
                        pointer const e { const_cast<pointer> ( pos ) + count };
                        while ( d.m.end != e )
                            *d.m.end++ = value;
                        std::memcpy ( ( void* ) ( d.m.end ), ( void* ) ( old + i0 ), i1 * sizeof ( value_type ) );
                        d.m.size = s1;
                        d.m.end += i1;
                        pdr::free ( old );
                    }
                    else {
                        pointer p = const_cast<pointer> ( pos );
                        const_pointer const e { p + count };
                        std::memmove ( ( void* ) e, ( void* ) p, ( d.m.end - pos ) * sizeof ( value_type ) );
                        while ( p != e )
                            *p++ = value;
                        d.m.size += count;
                        d.m.end += count;
                    }
                }
            }
            else {
                size_type const s1 = d.m.size + count; // s1 = new size.
                if ( d.m.size == d.m.capacity ) { // relocation.
                    pointer const old = d.m.end - d.m.size; // begin.
                    d.m.capacity = static_cast<size_type> ( growth_policy::grow_capacity_from ( s1 ) );
                    d.m.end = static_cast<pointer> ( pdr::malloc ( d.m.capacity * sizeof ( value_type ) ) );
                    size_type const i0 = pos - old, i1 = d.m.size - i0;
                    std::memcpy ( ( void* ) d.m.end, ( void* ) old, i0 * sizeof ( value_type ) );
                    pos = ( d.m.end += i0 );
                    pointer const e { const_cast<pointer> ( pos ) + count };
                    while ( d.m.end != e )
                        *d.m.end++ = value;
                    std::memcpy ( ( void* ) ( d.m.end ), ( void* ) ( old + i0 ), i1 * sizeof ( value_type ) );
                    d.m.size = s1;
                    d.m.end += i1;
                    pdr::free ( old );
                }
                else {
                    pointer const p = const_cast<pointer> ( pos );
                    const_pointer const e { p + count };
                    std::memmove ( ( void* ) e, ( void* ) p, ( d.m.end - pos ) * sizeof ( value_type ) );
                    while ( p != e )
                        *p++ = value;
                    d.m.size += count;
                    d.m.end += count;
                }
            }
        }
        return const_cast<iterator> ( pos );
    }
    template<typename InputIt, contiguous_input_iterator_t<InputIt> * = nullptr>
    [[ maybe_unused ]] iterator insert ( const_iterator pos, InputIt first, InputIt last ) {
        // std::cout << "iterator insert ( const_iterator pos, InputIt first, InputIt last ) ( contiguous )" << nl;
        return insert ( pos, &*first, static_cast<size_type> ( last - first ) );
    }
    template<typename InputIt, discontiguous_input_iterator_t<InputIt> * = nullptr>
    [[ maybe_unused ]] iterator insert ( const_iterator pos, InputIt first, InputIt last ) {
        // std::cout << "iterator insert ( const_iterator pos, InputIt first, InputIt last ) ( discontiguous )" << nl;
        const std::ptrdiff_t count = std::distance ( first, last ); // count values to be inserted.
        if ( count ) {
            if constexpr ( svo ( ) ) {
                if ( d.s.is_small ) {
                    std::size_t const s1 = d.s.size + count; // s1 = new size.
                    if ( ( s1 ) <= buff_size ( ) ) {
                        pointer p = const_cast<pointer> ( pos );
                        std::memmove ( ( void* ) ( p + count ), ( void* ) p, reinterpret_cast<char*> ( d.s.buffer + d.s.size ) - reinterpret_cast<char*> ( p ) );
                        d.s.size += count;
                        while ( first != last )
                            *p++ = *first++;
                    }
                    else {
                        podder const old ( *this );
                        size_type const c = static_cast<size_type> ( growth_policy::grow_capacity_from ( s1 ) );
                        new ( &d.m ) medium ( std::forward<medium> ( { s1, c, static_cast<pointer> ( pdr::malloc ( c * sizeof ( value_type ) ) ) } ) );
                        size_type const i0 = static_cast<size_type> ( pos - d.s.buffer );
                        std::memcpy ( ( void* ) d.m.end, ( void* ) old.d.s.buffer, i0 * sizeof ( value_type ) );
                        pos = ( d.m.end += i0 );
                        while ( first != last )
                            *d.m.end++ = *first++;
                        size_type const i1 = old.d.s.size - i0;
                        std::memcpy ( ( void* ) d.m.end, ( void* ) ( old.d.s.buffer + i0 ), i1 * sizeof ( value_type ) );
                        d.m.end += i1;
                    }
                }
                else { // medium.
                    std::size_t const s1 = d.m.size + count; // s1 = new size.
                    if ( d.m.size == d.m.capacity ) { // relocation.
                        pointer const old = d.m.end - d.m.size; // begin.
                        d.m.capacity = static_cast<size_type> ( growth_policy::grow_capacity_from ( s1 ) );
                        d.m.end = static_cast<pointer> ( pdr::malloc ( d.m.capacity * sizeof ( value_type ) ) );
                        size_type const i0 = pos - old, i1 = d.m.size - i0;
                        std::memcpy ( ( void* ) d.m.end, ( void* ) old, i0 * sizeof ( value_type ) );
                        pos = ( d.m.end += i0 );
                        while ( first != last )
                            *d.m.end++ = *first++;
                        std::memcpy ( ( void* ) ( d.m.end ), ( void* ) ( old + i0 ), i1 * sizeof ( value_type ) );
                        d.m.size = s1;
                        d.m.end += i1;
                        pdr::free ( old );
                    }
                    else { // no relocation.
                        pointer p = const_cast<pointer> ( pos );
                        std::memmove ( ( void* ) ( p + count ), ( void* ) p, count * sizeof ( value_type ) );
                        while ( first != last )
                            *p++ = *first++;
                        d.m.end += count;
                    }
                }
            } // non-svo.
            else {
                std::size_t const s1 = d.m.size + count; // s1 = new size.
                if ( d.m.size == d.m.capacity ) { // relocation.
                    pointer const old = d.m.end - d.m.size; // begin.
                    d.m.capacity = static_cast<size_type> ( growth_policy::grow_capacity_from ( s1 ) );
                    d.m.end = static_cast<pointer> ( pdr::malloc ( d.m.capacity * sizeof ( value_type ) ) );
                    size_type const i0 = pos - old, i1 = d.m.size - i0;
                    std::memcpy ( ( void* ) d.m.end, ( void* ) old, i0 * sizeof ( value_type ) );
                    pos = ( d.m.end += i0 );
                    while ( first != last )
                        *d.m.end++ = *first++;
                    std::memcpy ( ( void* ) ( d.m.end ), ( void* ) ( old + i0 ), i1 * sizeof ( value_type ) );
                    d.m.size = s1;
                    d.m.end += i1;
                    pdr::free ( old );
                }
                else { // no relocation.
                    pointer p = const_cast<pointer> ( pos );
                    std::memmove ( ( void* ) ( p + count ), ( void* ) p, count * sizeof ( value_type ) );
                    while ( first != last )
                        *p++ = *first++;
                    d.m.end += count;
                }
            }
        }
        return const_cast<iterator> ( pos );
    }
    iterator insert ( const_iterator pos, const_pointer first, size_type const count ) {
        // std::cout << "iterator insert ( const_iterator pos, const_pointer first, size_type const count )" << nl;
        if ( count ) {
            if constexpr ( svo ( ) ) {
                if ( d.s.is_small ) {
                    size_type const s1 = d.s.size + count; // s1 = new size.
                    if ( s1 <= buff_size ( ) ) {
                        pointer p = const_cast<pointer> ( pos );
                        std::memmove ( ( void* ) ( p + count ), ( void* ) p, reinterpret_cast<char*> ( d.s.buffer + d.s.size ) - reinterpret_cast<char*> ( p ) );
                        d.s.size += count;
                        std::memcpy ( ( void* ) p, ( void* ) first, count * sizeof ( value_type ) );
                    }
                    else {
                        podder const old ( *this );
                        size_type const c = static_cast<size_type> ( growth_policy::grow_capacity_from ( s1 ) );
                        new ( &d.m ) medium ( std::forward<medium> ( { s1, c, static_cast<pointer> ( pdr::malloc ( c * sizeof ( value_type ) ) ) } ) );
                        size_type const i0 = static_cast<size_type> ( pos - d.s.buffer );
                        std::memcpy ( ( void* ) d.m.end, ( void* ) old.d.s.buffer, i0 * sizeof ( value_type ) );
                        pos = ( d.m.end += i0 );
                        std::memcpy ( ( void* ) d.m.end, ( void* ) first, count * sizeof ( value_type ) );
                        d.m.end += count;
                        size_type const i1 = old.d.s.size - i0;
                        std::memcpy ( ( void* ) d.m.end, ( void* ) ( old.d.s.buffer + i0 ), i1 * sizeof ( value_type ) );
                        d.m.end += i1;
                    }
                }
                else { // medium.
                    size_type const s1 = d.m.size + count; // s1 = new size.
                    if ( d.m.size == d.m.capacity ) { // relocation.
                        pointer const old = d.m.end - d.m.size; // begin.
                        d.m.capacity = static_cast<size_type> ( growth_policy::grow_capacity_from ( s1 ) );
                        d.m.end = static_cast<pointer> ( pdr::malloc ( d.m.capacity * sizeof ( value_type ) ) );
                        size_type const i0 = pos - old, i1 = d.m.size - i0;
                        std::memcpy ( ( void* ) d.m.end, ( void* ) old, i0 * sizeof ( value_type ) );
                        d.m.end += i0;
                        pos = d.m.end;
                        std::memcpy ( ( void* ) d.m.end, ( void* ) first, count * sizeof ( value_type ) );
                        d.m.end += count;
                        std::memcpy ( ( void* ) d.m.end, ( void* ) ( old + i0 ), i1 * sizeof ( value_type ) );
                        d.m.size = s1;
                        d.m.end += i1;
                        pdr::free ( old );
                    }
                    else {
                        pointer const p = const_cast<pointer> ( pos );
                        std::memmove ( ( void* ) ( p + count ), ( void* ) p, ( d.m.end - pos ) * sizeof ( value_type ) );
                        std::memcpy ( ( void* ) p, ( void* ) first, count * sizeof ( value_type ) );
                        d.m.size += count;
                        d.m.end += count;
                    }
                }
            }
            else {
                size_type const s1 = d.m.size + count; // s1 = new size.
                if ( d.m.size == d.m.capacity ) { // relocation.
                    pointer const old = d.m.end - d.m.size; // begin.
                    d.m.capacity = static_cast<size_type> ( growth_policy::grow_capacity_from ( s1 ) );
                    d.m.end = static_cast<pointer> ( pdr::malloc ( d.m.capacity * sizeof ( value_type ) ) );
                    size_type const i0 = pos - old, i1 = d.m.size - i0;
                    std::memcpy ( ( void* ) d.m.end, ( void* ) old, i0 * sizeof ( value_type ) );
                    d.m.end += i0;
                    pos = d.m.end;
                    std::memcpy ( ( void* ) d.m.end, ( void* ) first, count * sizeof ( value_type ) );
                    d.m.end += count;
                    std::memcpy ( ( void* ) d.m.end, ( void* ) ( old + i0 ), i1 * sizeof ( value_type ) );
                    d.m.size = s1;
                    d.m.end += i1;
                    pdr::free ( old );
                }
                else {
                    pointer const p = const_cast<pointer> ( pos );
                    std::memmove ( ( void* ) ( p + count ), ( void* ) p, ( d.m.end - pos ) * sizeof ( value_type ) );
                    std::memcpy ( ( void* ) p, ( void* ) first, count * sizeof ( value_type ) );
                    d.m.size += count;
                    d.m.end += count;
                }
            }
        }
        return const_cast<iterator> ( pos );
    }
    [[ maybe_unused ]] iterator insert ( const_iterator pos, const_pointer first, const_pointer last ) {
        // std::cout << "iterator insert ( const_iterator pos, const_pointer first, const_pointer last )" << nl;
        return insert ( pos, first, static_cast<size_type> ( last - first ) );
    }
    [[ maybe_unused ]] iterator insert ( const_iterator pos, std::initializer_list<value_type> il ) {
        // std::cout << "iterator insert ( const_iterator pos, std::initializer_list<value_type> il )" << nl;
        return insert ( pos, il.begin ( ), static_cast<size_type> ( il.size ( ) ) );
    }

    // emplace.

    pointer mallocate ( size_type size ) {
        d.m.end = static_cast<pointer> ( pdr::malloc ( static_cast< std::size_t > ( size ) * sizeof ( value_type ) ) );
    }
    pointer reallocate ( pointer p, size_type size ) {
        d.m.end = static_cast<pointer> ( pdr::realloc ( d.m.end - d.m.size, ( d.m.capacity = growth_policy::grow_capacity_from ( d.m.size ) ) * sizeof ( value_type ) ) );
    }

    void deallocate ( ) noexcept {
        if ( not d.s.is_small )
            pdr::free ( d.m.end - d.m.size );
    }

    template<typename... Args>
    [[ maybe_unused ]] iterator emplace ( const_iterator pos, Args &&... args ) {
        // std::cout << "iterator emplace ( const_iterator pos, Args &&... args )" << nl;
        if constexpr ( svo ( ) ) {
            if ( d.s.is_small ) {
                if ( d.s.size < buff_size ( ) ) { // assign into small vector.
                    pointer const p = const_cast<pointer> ( pos );
                    std::memmove ( ( void* ) ( p + 1 ), ( void* ) p, reinterpret_cast<char*> ( d.s.buffer + d.s.size++ ) - reinterpret_cast<char*> ( p ) );
                    new ( p ) value_type { std::forward<Args> ( args )... };
                }
                else { // small vector -> medium vector.
                    size_type const c = growth_policy::grow_capacity_from ( buff_size ( ) );
                    pointer const p = static_cast<pointer> ( pdr::malloc ( static_cast< std::size_t > ( c ) * sizeof ( value_type ) ) );
                    pointer const pit = const_cast<pointer> ( pos );
                    std::memcpy ( ( void* ) p, ( void* ) d.s.buffer, reinterpret_cast<char*> ( pit ) - reinterpret_cast<char*> ( d.s.buffer ) );
                    new ( p + buff_size ( ) ) value_type { std::forward<Args> ( args )... };
                    std::memcpy ( ( void* ) ( p + ( pos - d.s.buffer ) ), ( void* ) pit, reinterpret_cast<char*> ( d.s.buffer + d.s.size ) - reinterpret_cast<char*> ( pit ) );
                    d.m.size = buff_size ( ) + 1;
                    d.m.capacity = static_cast< size_type > ( c );
                    d.m.end = p + buff_size ( ) + 1; // end points to back.
                    pos = p + ( pit - d.s.buffer ) - 1; // pointer to inserted value.
                }
            }
            else {
                if ( d.m.size == d.m.capacity ) { // relocate.
                    difference_type const idx = pos - ( d.m.end - d.m.size );
                    d.m.end = static_cast<pointer> ( pdr::realloc ( d.m.end - d.m.size, ( d.m.capacity = growth_policy::grow_capacity_from ( d.m.size ) ) * sizeof ( value_type ) ) );
                    pointer const p = d.m.end + idx;
                    d.m.end += ++d.m.size;
                    std::memmove ( ( void* ) ( p + 1 ), ( void* ) ( p ), reinterpret_cast<char*> ( d.m.end ) - reinterpret_cast<char*> ( p ) );
                    pos = new ( p ) value_type { std::forward<Args> ( args )... };
                }
                else {
                    pointer const p = const_cast<pointer> ( pos );
                    ++d.m.size;
                    std::memmove ( ( void* ) ( p + 1 ), ( void* ) ( p ), reinterpret_cast<char*> ( d.m.end++ ) - reinterpret_cast<char*> ( p ) );
                    new ( p ) value_type { std::forward<Args> ( args )... };
                }
            }
        }
        else { // non-svo.
            if ( d.m.size == d.m.capacity ) { // relocate.
                difference_type const idx = pos - ( d.m.end - d.m.size );
                d.m.end = static_cast<pointer> ( pdr::realloc ( d.m.end - d.m.size, ( d.m.capacity = growth_policy::grow_capacity_from ( d.m.size ) ) * sizeof ( value_type ) ) );
                pointer const p = d.m.end + idx;
                d.m.end += ++d.m.size;
                std::memmove ( ( void* ) ( p + 1 ), ( void* ) ( p ), reinterpret_cast<char*> ( d.m.end ) - reinterpret_cast<char*> ( p ) );
                pos = new ( p ) value_type { std::forward<Args> ( args )... };
            }
            else {
                pointer const p = const_cast<pointer> ( pos );
                ++d.m.size;
                std::memmove ( ( void* ) ( p + 1 ), ( void* ) ( p ), reinterpret_cast<char*> ( d.m.end++ ) - reinterpret_cast<char*> ( p ) );
                new ( p ) value_type { std::forward<Args> ( args )... };
            }
        }
        return const_cast<iterator> ( pos );
    }

    // erase.

    PRIVATE

    void unchecked_erase_small_impl ( pointer pos ) noexcept {  // good for no_svo types.
        std::memmove ( ( void* ) pos, ( void* ) ( pos + 1 ), reinterpret_cast<char*> ( d.s.buffer + d.s.size-- ) - reinterpret_cast<char*> ( pos ) );
    }

    void unchecked_erase_medium_impl ( pointer pos ) noexcept {
        std::memmove ( ( void* ) pos, ( void* ) ( pos + 1 ), reinterpret_cast<char*> ( d.m.end ) - reinterpret_cast<char*> ( pos ) );
        --d.m.size;
        --d.m.end;
    }

    PUBLIC

    void unchecked_erase ( iterator pos ) noexcept {  // good for no_svo types.
        if ( d.s.is_small )
            unchecked_erase_small_impl ( static_cast<pointer> ( pos ) );
        else
            unchecked_erase_medium_impl ( static_cast<pointer> ( pos ) );
    }

    void erase ( iterator pos ) noexcept {
        if ( size ( ) )
            unchecked_erase ( pos );
    }

    PRIVATE

    bool not_have_duplicates ( const_reference value ) const noexcept pure_function {
        bool not_found = true;
        pointer p = begin_pointer ( ), e = end_pointer ( );
        while ( p != e ) {
            if ( value == *p ) {
                if ( not_found ) {
                    not_found = false;
                }
                else {
                    return false;
                }
            }
            ++p;
        }
        return true;
    }

    bool have_duplicates ( const_reference value ) const noexcept pure_function {
        return not not_have_duplicates ( value );
    }

    PUBLIC

    // unordered_erase.

    // A  Nike(tm) function (Just Do It!). Replaces the to be erased value
    // with the back () value and decrements size, no checking of any kind.
    // Returns an iterator to the value after the erased value.
    [[ maybe_unused ]] iterator unchecked_unordered_erase ( iterator pos ) noexcept {
        --d.m.size;
        if ( d.s.is_small )
            *pos = d.s.buffer [ d.m.size ];
        else
            *pos = *( --d.m.end );
        return ++pos;
    }

    // Does not maintain order. Only works correctly iff there
    // are no value duplicates. This requirement is asserted.
    void unordered_erase_unique ( const_reference value ) noexcept {
        if constexpr ( is_debug::value ) {
            if ( have_duplicates ( value ) )
                throw std::runtime_error ( "unordered_erase_unique is applied to a podder containing duplicates of some of it's values" );
        }
        pointer p = begin_pointer ( );
        while ( p <= d.m.end ) {
            if ( *p == value ) {
                --d.m.size;
                *p = *( --d.m.end );
                return;
            }
            ++p;
        }
    }

    // unordered erase.

    // Does not maintain order. Erases all duplicates of value.
    void unordered_erase ( const_reference value ) noexcept {
        pointer p = begin_pointer ( );
        while ( p <= d.m.end ) { // do a linear search.
            if ( *p == value ) {
                do {
                    --d.m.size;
                    --d.m.end;
                    if ( p == d.m.end )
                        return;
                } while ( value == *d.m.end );
                *p = *d.m.end;
            }
            ++p;
        }
    }

    // push_front.

    [[ maybe_unused ]] reference push_front ( const_reference value ) {
        return insert ( begin ( ), value_type { value } );
    }
    [[ maybe_unused ]] reference push_front ( rv_reference value ) {
        return insert ( begin ( ), std::forward<value_type> ( value ) );
    }

    // push_back.

    [[ maybe_unused ]] reference push_back ( const_reference value ) {
        return emplace_back ( value_type { value } );
    }
    [[ maybe_unused ]] reference push_back ( rv_reference value ) {
        return emplace_back ( std::forward<value_type> ( value ) );
    }

    // emplace_front.

    template<typename... Args>
    [[ maybe_unused ]] reference emplace_front ( Args &&... args ) {
        return insert ( begin ( ), value_type { std::forward<Args> ( args )... } );
    }

    // emplace_back.

    template<typename... Args>
    [[ maybe_unused ]] reference emplace_back ( Args && ...args ) { // it's ugly (and code duplication), the nested if/then's, but it's the fastest (faster than gcc-style jump-table).
        pointer pos = nullptr;
        if constexpr ( svo ( ) ) {
            if ( d.s.is_small ) { // assign into svo vector.
                if ( d.s.size < buff_size ( ) ) { // assign into small vector.
                    assert ( d.s.size <= buff_size ( ) );
                    pos = new ( d.s.buffer + d.s.size++ ) value_type { std::forward<Args> ( args )... };
                }
                else { // small vector -> medium vector, assign into medium vector.
                    size_type const c = growth_policy::grow_capacity_from ( buff_size ( ) );
                    pointer p = static_cast<pointer> ( pdr::malloc ( static_cast<std::size_t> ( c ) * sizeof ( value_type ) ) );
                    std::memcpy ( ( void* ) p, ( void* ) d.s.buffer, buff_size ( ) * sizeof ( value_type ) );
                    p += buff_size ( );
                    new ( & d.m ) medium { buff_size ( ) + 1, c, p + 1 };
                    assert ( ( buff_size ( ) + 1 ) <= d.m.capacity );
                    assert ( d.m.size <= d.m.capacity );
                    pos = new ( p ) value_type { std::forward<Args> ( args )... };
                }
            }
            else {
                if ( d.m.size == d.m.capacity ) { // relocate.
                     d.m.end = static_cast<pointer> ( pdr::realloc ( d.m.end - d.m.size, ( d.m.capacity = growth_policy::grow_capacity_from ( d.m.size ) ) * sizeof ( value_type ) ) ) + d.m.size;
                 }
                ++d.m.size;
                assert ( d.m.size <= d.m.capacity );
                assert ( buff_size ( ) < d.m.capacity );
                pos = new ( d.m.end++ ) value_type { std::forward<Args> ( args )... };
            }
        }
        else { // assign into non-svo (medium) vector.
            if ( d.m.size == d.m.capacity ) { // not allocated or relocate.
                if ( d.m.capacity ) // relocate.
                    d.m.end = static_cast<pointer> ( pdr::realloc ( d.m.end - d.m.size, ( d.m.capacity = growth_policy::grow_capacity_from ( d.m.size ) ) * sizeof ( value_type ) ) ) + d.m.size;
                else // allocate.
                    d.m.end = static_cast<pointer> ( pdr::malloc ( ( d.m.capacity = growth_policy::grow_capacity_from ( ) ) * sizeof ( value_type ) ) ) + d.m.size;
            }
            ++d.m.size;
            pos = new ( d.m.end++ ) value_type { std::forward<Args> ( args )... };
        }
        return *pos;
    }

    // pop_front.

    void pop_front ( ) {
        erase ( begin ( ) );
    }

    void unchecked_pop_front ( ) {
        unchecked_erase ( begin ( ) );
    }

    [[ nodiscard ]] optional_value_type pop_front_get ( ) {
        /*
        if ( d.s.is_small ) {
        unchecked_erase_small_impl ( p.begin ( ), p );
        }
        else {
        unchecked_erase_medium_impl ( p.begin ( ), p );
        }

        if ( size ( ) ) {
        optional_value_type value { front ( ) };
        unchecked_erase ( p );
        return value;
        }
        else {
        return optional_value_type { };
        }
        */
    }

    // pop_back.

    void unchecked_pop_back ( ) noexcept {
        if ( d.s.is_small ) {
            --d.s.size;
        }
        else {
            --d.m.size;
            --d.m.end;
        }
    }

    [[ nodiscard ]] value_type unchecked_pop_back_get ( ) noexcept {
        if ( d.s.is_small ) {
            return d.s.buffer [ --d.s.size ];
        }
        else {
            --d.m.size;
            return *d.m.end--;
        }
    }

    void pop_back ( ) noexcept {
        if ( d.s.is_small ) {
            if ( d.s.size )
                --d.s.size;
        }
        else {
            if ( d.m.size ) {
                --d.m.size;
                --d.m.end;
            }
        }
    }

    // pops back and returns std::optional of the popped value.
    [[ nodiscard ]] optional_value_type pop_back_get ( ) noexcept {
        if ( d.s.is_small ) {
            if ( d.s.size )
                return optional_value_type { d.s.buffer [ --d.s.size ] };
            else
                return optional_value_type { };
        }
        else {
            if ( d.m.size ) {
                --d.m.size;
                return optional_value_type { *d.m.end-- };
            }
            else {
                return optional_value_type { };
            }
        }
    }

    PRIVATE

    void resize_reserve_only_impl ( size_type const size, bool const construct ) {
        if constexpr ( svo ( ) ) {
            if ( d.s.is_small ) {
                if ( size <= buff_size ( ) ) {
                    d.s.size = size;
                    if ( construct ) {
                        if ( size > d.s.size ) {
                            pointer p = d.s.buffer + d.s.size, e = d.s.buffer + size;
                            while ( p != e )
                                new ( p++ ) value_type { };
                        }
                    }
                }
                else {
                    pointer const p = static_cast<pointer> ( pdr::malloc ( static_cast<std::size_t> ( size ) * sizeof ( value_type ) ) );
                    std::memcpy ( ( void* ) p, ( void* ) d.s.buffer, d.s.size * sizeof ( value_type ) );
                    d.m.size = static_cast<size_type> ( d.s.size );
                    d.m.capacity = size;
                    d.m.end = p + d.m.size;
                    if ( construct ) {
                        if ( size > d.s.size ) {
                            pointer p = d.m.end, e = d.m.end - d.m.size + size;
                            while ( p != e )
                                new ( p++ ) value_type { };
                        }
                    }
                }
            }
            else {
                if ( size <= d.m.capacity ) {
                    d.m.size = size;
                }
                else { // relocate.
                    pointer const p = d.m.end - d.m.size;
                    d.m.end = static_cast<pointer> ( pdr::malloc ( p * ( d.m.capacity = size ) * sizeof ( value_type ) ) );
                    std::memcpy ( ( void* ) d.m.end, ( void* ) p, d.m.size * sizeof ( value_type ) );
                    d.m.end += ( d.m.size = size );
                }
                if ( construct ) {
                    if ( size > d.s.size ) {
                        pointer p = d.m.end, e = d.m.end - d.m.size + size;
                        while ( p != e )
                            new ( p++ ) value_type { };
                    }
                }
            }
        }
        else {
            if ( size > d.m.capacity ) { // not allocated or relocate.
                if ( d.m.capacity ) { // it is allocated.
                    pointer const p = d.m.end - d.m.size;
                    d.m.end = static_cast<pointer> ( pdr::malloc ( p * ( d.m.capacity = size ) * sizeof ( value_type ) ) );
                    std::memcpy ( ( void* ) d.m.end, ( void* ) p, d.m.size * sizeof ( value_type ) );
                    d.m.end += d.m.size;
                }
                else { // allocate.
                    d.m.end = static_cast<pointer> ( pdr::malloc ( ( d.m.capacity = size ) * sizeof ( value_type ) ) ) + d.m.size;
                }
            }
            if ( construct ) {
                if ( size > d.s.size ) {
                    pointer p = d.m.end, e = d.m.end - d.m.size + size;
                    while ( p != e )
                        new ( p++ ) value_type { };
                }
            }
        }
    }

    PUBLIC

    // resize.

    void resize_reserve_only ( size_type const size ) {
        resize_reserve_impl ( size, false );
    }

    void resize ( size_type const size ) {
        resize_reserve_impl ( size, true );
    }

    // swap.

    // Swap function, function (possibly) invalidates any references, pointers, or
    // iterators referring to the elements of the containers being swapped.
    void swap ( podder & p ) noexcept {
        // the small case treated 'as if' medium.
        std::swap ( d.m.size, p.d.m.size );
        std::swap ( d.m.capacity, p.d.m.capacity );
        std::swap ( d.m.end, p.d.m.end );
    }

    // comparison.

    template<typename Container, contiguous_container_t<Container> * = nullptr>
    [[ nodiscard ]] bool operator == ( Container & rhs ) const noexcept {
        bool value = false;
        if constexpr ( svo ( ) ) {
            if ( d.b.high ) {
                if ( d.s.size == rhs.size ( ) )
                    value = std::memcmp ( ( void* ) d.s.buffer, ( void* ) rhs.data ( ), rhs.size ( ) * sizeof ( value_type ) ) == 0;
            }
            else {
                if ( d.m.size == rhs.size ( ) )
                    value = std::memcmp ( ( void* ) ( d.m.end - d.m.size ), ( void* ) rhs.data ( ), rhs.size ( ) * sizeof ( value_type ) ) == 0;
            }
        }
        else {
            if ( d.m.size == rhs.size ( ) )
                value = std::memcmp ( ( void* ) ( d.m.end - d.m.size ), ( void* ) rhs.data ( ), rhs.size ( ) * sizeof ( value_type ) ) == 0;
        }
        return value;
    }

    template<typename Container, contiguous_container_t<Container> * = nullptr>
    [[ nodiscard ]] bool operator != ( Container & rhs ) const noexcept {
        return not ( *this == rhs );
    }

    template<typename Container, contiguous_container_t<Container> * = nullptr>
    [[ nodiscard ]] bool operator < ( Container & rhs ) const noexcept {
        bool value = false;
        if constexpr ( svo ( ) ) {
            if ( d.b.high )
                value = std::memcmp ( ( void* ) d.s.buffer, ( void* ) rhs.data ( ), std::min ( static_cast<std::size_t> ( d.s.size ), rhs.size ( ) ) * sizeof ( value_type ) ) < 0;
            else
                value = std::memcmp ( ( void* ) ( d.m.end - d.m.size ), ( void* ) rhs.data ( ), std::min ( static_cast<std::size_t> ( d.m.size ), rhs.size ( ) ) * sizeof ( value_type ) ) < 0;
        }
        else {
            value = std::memcmp ( ( void* ) ( d.m.end - d.m.size ), ( void* ) rhs.data ( ), std::min ( static_cast<std::size_t> ( d.m.size ), rhs.size ( ) ) * sizeof ( value_type ) ) < 0;
        }
        return value;
    }

    template<typename Container, contiguous_container_t<Container> * = nullptr>
    [[ nodiscard ]] bool operator > ( Container & rhs ) const noexcept {
        bool value = false;
        if constexpr ( svo ( ) ) {
            if ( d.b.high )
                value = std::memcmp ( ( void* ) d.s.buffer, ( void* ) rhs.data ( ), std::min ( static_cast<std::size_t> ( d.s.size ), rhs.size ( ) ) * sizeof ( value_type ) ) > 0;
            else
                value = std::memcmp ( ( void* ) ( d.m.end - d.m.size ), ( void* ) rhs.data ( ), std::min ( static_cast<std::size_t> ( d.m.size ), rhs.size ( ) ) * sizeof ( value_type ) ) > 0;
        }
        else {
            value = std::memcmp ( ( void* ) ( d.m.end - d.m.size ), ( void* ) rhs.data ( ), std::min ( static_cast<std::size_t> ( d.m.size ), rhs.size ( ) ) * sizeof ( value_type ) ) > 0;
        }
        return value;
    }

    template<typename Container, contiguous_container_t<Container> * = nullptr>
    [[ nodiscard ]] bool operator <= ( Container & rhs ) const noexcept {
        return not ( *this > rhs );
    }

    template<typename Container, contiguous_container_t<Container> * = nullptr>
    [[ nodiscard ]] bool operator >= ( Container & rhs ) const noexcept {
        return not ( *this < rhs );
    }

    template<typename Container, discontiguous_container_t<Container> * = nullptr>
    [[ nodiscard ]] bool operator == ( Container & rhs ) const noexcept {
        if ( size ( ) != rhs.size ( ) ) {
            return false;
        }
        return std::equal ( begin ( ), end ( ), std::begin ( rhs ), std::end ( rhs ) );
    }

    template<typename Container, discontiguous_container_t<Container> * = nullptr>
    [[ nodiscard ]] bool operator != ( Container & rhs ) const noexcept {
        return not ( *this == rhs );
    }

    template<typename Container, discontiguous_container_t<Container> * = nullptr>
    [[ nodiscard ]] bool operator < ( Container & rhs ) const noexcept {
        return std::lexicographical_compare ( begin ( ), end ( ), std::begin ( rhs ), std::end ( rhs ), std::less<value_type> ( ) );
    }

    template<typename Container, discontiguous_container_t<Container> * = nullptr>
    [[ nodiscard ]] bool operator > ( Container & rhs ) const noexcept {
        return std::lexicographical_compare ( begin ( ), end ( ), std::begin ( rhs ), std::end ( rhs ), std::greater<value_type> ( ) );
    }

    template<typename Container, discontiguous_container_t<Container> * = nullptr>
    [[ nodiscard ]] bool operator <= ( Container & rhs ) const noexcept {
        return not ( *this > rhs );
    }

    template<typename Container, discontiguous_container_t<Container> * = nullptr>
    [[ nodiscard ]] bool operator >= ( Container & rhs ) const noexcept {
        return not ( *this < rhs );
    }

    [[ nodiscard ]] bool operator == ( podder & rhs ) const noexcept {
        bool value = false;
        if constexpr ( svo ( ) ) {
            if ( d.b.high ) {
                if ( rhs.d.b.high ) { // small - small.
                    if ( d.b.high == rhs.d.b.high )
                        value = std::memcmp ( ( void* ) d.s.buffer, ( void* ) rhs.d.s.buffer, d.s.size * sizeof ( value_type ) ) == 0;
                }
                else // small - medium.
                    if ( d.s.size == rhs.d.m.size )
                        value = std::memcmp ( ( void* ) d.s.buffer, ( void* ) ( rhs.d.m.end - rhs.d.m.size ), rhs.d.m.size * sizeof ( value_type ) ) == 0;
            }
            else {
                if ( not rhs.d.b.high ) { // medium - medium.
                    if ( d.m.size == rhs.d.m.size )
                        value = std::memcmp ( ( void* ) ( d.m.end - d.m.size ), ( void* ) ( rhs.d.m.end - rhs.d.m.size ), d.m.size * sizeof ( value_type ) ) == 0;
                }
                else // medium - small.
                    if ( d.m.size == rhs.d.s.size )
                        value = std::memcmp ( ( void* ) ( d.m.end - d.m.size ), ( void* ) rhs.d.s.buffer, d.m.size * sizeof ( value_type ) ) == 0;
            }
        }
        else {
            if ( d.m.size == rhs.d.m.size ) // medium - medium.
                value = std::memcmp ( ( void* ) ( d.m.end - d.m.size ), ( void* ) ( rhs.d.m.end - rhs.d.m.size ), d.m.size * sizeof ( value_type ) ) == 0;
        }
        return value;
    }

    [[ nodiscard ]] bool operator != ( podder & rhs ) const noexcept {
        return not ( *this == rhs );
    }

    [[ nodiscard ]] bool operator < ( podder & rhs ) const noexcept {
        bool value = false;
        if constexpr ( svo ( ) ) {
            if ( d.b.high ) {
                if ( rhs.d.b.high ) // small - small.
                    value = std::memcmp ( ( void* ) d.s.buffer, ( void* ) rhs.d.s.buffer, std::min ( d.s.size, rhs.d.s.size ) * sizeof ( value_type ) ) < 0;
                else // small - medium.
                    value = std::memcmp ( ( void* ) d.s.buffer, ( void* ) ( rhs.d.m.end - rhs.d.m.size ), std::min ( static_cast<size_type> ( d.s.size ), rhs.d.m.size ) * sizeof ( value_type ) ) < 0;
            }
            else {
                if ( not rhs.d.b.high ) // medium - medium.
                    value = std::memcmp ( ( void* ) ( d.m.end - d.m.size ), ( void* ) ( rhs.d.m.end - rhs.d.m.size ), std::min ( d.m.size, rhs.d.m.size ) * sizeof ( value_type ) ) < 0;
                else // medium - small.
                    value = std::memcmp ( ( void* ) ( d.m.end - d.m.size ), ( void* ) rhs.d.s.buffer, std::min ( d.m.size, static_cast<size_type> ( rhs.d.s.size ) ) * sizeof ( value_type ) ) < 0;
             }
        }
        else {
            value = std::memcmp ( ( void* ) ( d.m.end - d.m.size ), ( void* ) ( rhs.d.m.end - rhs.d.m.size ), std::min ( d.m.size, rhs.d.m.size ) * sizeof ( value_type ) ) < 0;
        }
        return value;
    }

    [[ nodiscard ]] bool operator > ( podder & rhs ) const noexcept {
        bool value = false;
        if constexpr ( svo ( ) ) {
            if ( d.b.high ) {
                if ( rhs.d.b.high ) // small - small.
                    value = std::memcmp ( ( void* ) d.s.buffer, ( void* ) rhs.d.s.buffer, std::min ( d.s.size, rhs.d.s.size ) * sizeof ( value_type ) ) > 0;
                else // small - medium.
                    value = std::memcmp ( ( void* ) d.s.buffer, ( void* ) ( rhs.d.m.end - rhs.d.m.size ), std::min ( static_cast<size_type> ( d.s.size ), rhs.d.m.size ) * sizeof ( value_type ) ) > 0;
            }
            else {
                if ( not rhs.d.b.high ) // medium - medium.
                    value = std::memcmp ( ( void* ) ( d.m.end - d.m.size ), ( void* ) ( rhs.d.m.end - rhs.d.m.size ), std::min ( d.m.size, rhs.d.m.size ) * sizeof ( value_type ) ) > 0;
                else // medium - small.
                    value = std::memcmp ( ( void* ) ( d.m.end - d.m.size ), ( void* ) rhs.d.s.buffer, std::min ( d.m.size, static_cast<size_type> ( rhs.d.s.size ) ) * sizeof ( value_type ) ) > 0;
            }
        }
        else {
            value = std::memcmp ( ( void* ) ( d.m.end - d.m.size ), ( void* ) ( rhs.d.m.end - rhs.d.m.size ), std::min ( d.m.size, rhs.d.m.size ) * sizeof ( value_type ) ) > 0;
        }
        return value;
    }

    [[ nodiscard ]] bool operator <= ( podder & rhs ) const noexcept {
        return not ( *this > rhs );
    }

    [[ nodiscard ]] bool operator >= ( podder & rhs ) const noexcept {
        return not ( *this < rhs );
    }

    [[ nodiscard ]] constexpr size_type size_in_bytes ( ) const noexcept {
        return size ( ) * size_type { sizeof ( value_type ) };
    }
    [[ nodiscard ]] constexpr size_type capacity_in_bytes ( ) const noexcept {
        return capacity ( ) * size_type { sizeof ( value_type ) };
    }

    PRIVATE

    [[ nodiscard ]] pointer begin_pointer ( ) noexcept {
        if constexpr ( svo ( ) ) {
            return d.s.is_small ? d.s.buffer : d.m.end - d.m.size;
        }
        else {
            return d.m.end - d.m.size;
        }
    }
    [[ nodiscard ]] const_pointer begin_pointer ( ) const noexcept {
        if constexpr ( svo ( ) ) {
            return d.s.is_small ? d.s.buffer : d.m.end - d.m.size;
        }
        else {
            return d.m.end - d.m.size;
        }
    }

    [[ nodiscard ]] pointer end_pointer ( ) noexcept {
        if constexpr ( svo ( ) ) {
            return d.s.is_small ? d.s.buffer + d.s.size : d.m.end;
        }
        else {
            return d.m.end;
        }
    }
    [[ nodiscard ]] const_pointer end_pointer ( ) const noexcept {
        if constexpr ( svo ( ) ) {
            return d.s.is_small ? d.s.buffer + d.s.size : d.m.end;
        }
        else {
            return d.m.end;
        }
    }

    [[ nodiscard ]] pointer back_pointer ( ) noexcept {
        return end_pointer ( ) - 1;
    }
    [[ nodiscard ]] const_pointer back_pointer ( ) const noexcept {
        return end_pointer ( ) - 1;
    }

    // SVO d structures.

    PUBLIC

    [[ nodiscard ]] static constexpr bool svo ( ) noexcept {
        return static_cast<bool> ( ( sizeof ( medium_s ) - std::size_t { 1 } ) / sizeof ( value_type ) );
    }

    [[ nodiscard ]] svo_type svo_model ( ) const noexcept {
        if constexpr ( svo ( ) ) {
            if ( d.s.is_small )
                return svo_type::small;
        }
        return svo_type::medium;
    }

    [[ nodiscard ]] static constexpr size_type svo_capacity ( ) noexcept {
        if constexpr ( svo ( ) ) {
            return static_cast<size_type> ( ( sizeof ( medium_s ) - std::size_t { 1 } ) / sizeof ( value_type ) );
        }
        else {
            return size_type { 0 };
        }
    }

    PRIVATE

    [[ nodiscard ]] static constexpr std::size_t buff_size ( ) noexcept {
        if constexpr ( svo ( ) ) {
            return ( sizeof ( medium_s ) - std::size_t { 1 } ) / sizeof ( value_type );
        }
        else {
            return std::size_t { 1 }; // cannot have 0-sized array (without the use of extensions).
        }
    }

    // padd_size ( ) == sizeof ( medium_s ), implies value_type doesn't fit in buffer
    // and selects the template<> struct small_s<sizeof ( medium_s )> specialization, which
    // goes unused, but needs to be defined and valid.
    [[ nodiscard ]] static constexpr int padd_size ( ) noexcept {
        if constexpr ( svo ( ) ) {
            return static_cast<int> ( sizeof ( medium_s ) - buff_size ( ) * sizeof ( value_type ) ) - 1;
        }
        else {
            return -1;
        }
    }

    [[ nodiscard ]] static constexpr int svo_size_type_width ( ) noexcept {
        return 4 + ( sizeof ( std::uintptr_t ) > 32u );
    }


    struct large_s {
        size_type size;
        size_type capacity;
        tagged_pointer<value_type> end;
    };

    struct medium_s {
        size_type size;
        size_type capacity;
        pointer end;
    };

    template<int PaddSize>
    struct small_s {
        value_type buffer [ buff_size ( ) ];
        value_type : PaddSize * 8;
        value_type size : 5;
        value_type is_small : 1;
        value_type : 2;
    };

    template<>
    struct small_s<0> { // for std::uint8_t size value_type's, fend of padding [ 0 ] warnings.
        value_type buffer [ buff_size ( ) ];
        value_type size : 5;
        value_type is_small : 1;
        value_type : 2;
    };

    template<>
    struct small_s<-1> { // dummy type to deal whith objects that don't fit in the buffer.
        std::uint8_t _ [ sizeof ( medium_s ) ];
    };

    struct byte_view_s {
        std::uint8_t _ [ sizeof ( medium_s ) - 1 ];
        std::uint8_t high; // the highest byte.
    };


    using large     = large_s;
    using medium    = medium_s;
    using small     = small_s < padd_size ( ) >;
    using byte_view = byte_view_s; // byte view.

    union podder_data_u {

        large      l;
        medium     m;
        small      s;
        byte_view  b;

        podder_data_u ( ) noexcept : l ( ) { }
    };

    using podder_data = podder_data_u;

    void medium_clear ( ) noexcept {
        d.m.end -= d.m.size;
        if constexpr ( is_debug::value ) {
            std::memset ( ( void* ) d.m.end, 0, d.m.capacity * sizeof ( value_type ) );
        }
        else {
            d.m.size = 0;
        }
    }
    void small_clear ( ) noexcept {
        if constexpr ( is_debug::value ) {
            std::memset ( ( void* ) this, 0, sizeof ( podder ) - 1 );
        }
        d.b.high = 0b0010'0000;
    }

    void clear_to_small ( ) noexcept {
        if ( not ( d.s.is_small ) ) {
            pdr::free ( d.m.end - d.m.size );
        }
        small_clear ( );
    }

    constexpr void set_small_size ( size_type const s ) noexcept {
        assert ( s <= buff_size ( ) );
        d.b.high = static_cast<std::uint8_t> ( s ) | 0b0010'0000;
    }

    void print_svo ( ) const noexcept {
        if ( d.s.is_small ) {
            std::cout << "svo      : small" << nl;
            std::cout << "size     : " << ( int ) d.s.size << nl;
            std::cout << "capacity : " << buff_size ( ) << nl;
            std::cout << "data     : "; std::printf ( "%p", ( void* ) d.s.buffer ); std::cout << nl;
        }
        else {
            std::cout << "svo      : medium" << nl;
            std::cout << "size     : " << d.m.size << nl;
            std::cout << "capacity : " << d.m.capacity << nl;
            std::cout << "data     : "; std::printf ( "%p", ( void* ) ( d.m.end - d.m.size ) ); std::cout << nl;
        }
    }

    podder_data d;
};


// additional deduction guide.
///template<typename InIter>
//podder ( InIter b, InIter e )->podder<typename std::iterator_traits<InIter>::value_type>;


// swap function, function (possibly) invalidates any references, pointers, or
// iterators referring to the elements of the containers being swapped.
template<typename Type, typename SizeType = std::size_t, typename GrowthPolicy = visual_studio_growth_policy<SizeType>>
void swap ( podder<Type, SizeType, GrowthPolicy> & a, podder<Type, SizeType, GrowthPolicy> & b ) noexcept {
    a.swap ( b );
}


template<typename T>
void print ( podder<T> & p ) {
    p.print_svo ( );
    std::cout << "values   : ";
    for ( auto value : p ) {
        std::cout << ( std::int64_t ) value << ' ';
    }
    std::cout << nl;
}


namespace std {

template<typename Type = std::uint8_t, typename SizeType = std::size_t, typename GrowthPolicy = visual_studio_growth_policy<SizeType>>
void swap ( podder<Type, SizeType, GrowthPolicy> &, podder<Type, SizeType, GrowthPolicy> & ) {
    throw std::domain_error ( std::string ( "podder is not std-compliant, and std::swap ( podder & a, podder & b ) has not" ) +
                              std::string ( "\n" ) +
                              std::string ( "been implemented. However, a pdr::swap ( podder & a, podder & b ) is provided," ) +
                              std::string ( "\n" ) +
                              std::string ( "which will invalidate references, pointers, and iterators into the d being" ) +
                              std::string ( "\n" ) +
                              std::string ( "swapped." ) +
                              std::string ( "\n" )
    );
}

}

#undef PRIVATE
#undef PUBLIC

#ifdef podder_undef_small
#define small podder_undef_small
#undef podder_undef_small
#endif

#ifdef pure_function
#undef pure_function
#endif
