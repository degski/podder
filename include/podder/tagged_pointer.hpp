
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

#include <cstdint>

#include <limits>
#include <sax/iostream.hpp>

template<typename ValueType>
struct tagged_pointer {

    using value_type = ValueType;
    using pointer    = value_type *;

    enum class allocation_type : std::uint8_t { large, medium, small };

    private:
    template<typename VT>
    union tagged_ptr_u {

        using value_type = VT;
        using pointer    = value_type *;

        [[nodiscard]] static constexpr int svo_size_type_width ( ) noexcept { return 4 + ( sizeof ( std::uintptr_t ) > 32u ); }

        struct tag {
            std::uintptr_t ptr : static_cast<int> ( sizeof ( std::uintptr_t ) * 8 - 8 );
            std::uintptr_t : 7;
            std::uintptr_t is_large : 1;
        };

        tagged_ptr_u ( ) noexcept {}
        tagged_ptr_u ( pointer p ) noexcept : ptr ( p ) {}
        tagged_ptr_u ( pointer p, bool b ) noexcept : ptr ( p ) { tag.is_large = b; }

        pointer ptr = nullptr;
        tag tag;
    };

    using tagged_ptr_t = tagged_ptr_u<ValueType>;

    tagged_ptr_t tp;

    public:
    tagged_pointer ( ) noexcept {}
    tagged_pointer ( tagged_pointer const & tp_ ) noexcept : tp ( tp_.tp ) {}
    explicit tagged_pointer ( pointer p ) noexcept : tp ( p ) {}
    explicit tagged_pointer ( pointer p, bool b ) noexcept : tp ( p ) { tp.tag.is_large = b; }

    constexpr void large ( bool b ) noexcept { tp.tag.is_large = b; }
    [[nodiscard]] constexpr bool is_large ( ) const noexcept { return tp.tag.is_large; }

    [[nodiscard]] constexpr tagged_pointer & operator= ( tagged_pointer const & rhs ) noexcept {
        tp.ptr = rhs.tp.ptr;
        return *this;
    }

    [[nodiscard]] constexpr tagged_pointer & operator= ( pointer const rhs ) noexcept {
        tp.ptr = rhs;
        return *this;
    }

    [[maybe_unused]] constexpr tagged_pointer & operator-- ( ) noexcept {
        assert ( tp.tag.ptr > sizeof ( value_type ) );
        tp.tag.ptr -= sizeof ( value_type );
        return *this;
    }

    [[maybe_unused]] constexpr tagged_pointer operator-- ( int ) noexcept {
        const tagged_pointer tmp ( *this );
        operator-- ( );
        return tmp;
    }

    [[nodiscard]] constexpr std::ptrdiff_t operator- ( tagged_pointer const & rhs ) const noexcept {
        assert ( tp.tag.ptr > rhs.tp.tag.ptr );
        return static_cast<pointer> ( tp.tag.ptr ) - static_cast<pointer> ( rhs.tp.tag.ptr );
    }

    [[nodiscard]] constexpr std::ptrdiff_t operator- ( pointer const rhs ) const noexcept {
        assert ( static_cast<pointer> ( tp.tag.ptr ) > rhs );
        return static_cast<pointer> ( tp.tag.ptr ) - rhs;
    }

    // return a pointer.
    template<typename Int, typename = std::enable_if_t<std::is_integral<Int>::value>>
    [[nodiscard]] constexpr pointer operator- ( Int const rhs ) const noexcept {
        assert ( tp.tag.ptr > rhs * sizeof ( value_type ) );
        return static_cast<pointer> ( tp.tag.ptr ) - rhs;
    }

    // return a tagged_pointer.
    template<typename Int, typename = std::enable_if_t<std::is_integral<Int>::value>>
    [[maybe_unused]] constexpr tagged_pointer & operator-= ( Int const rhs ) noexcept {
        assert ( tp.tag.ptr > rhs * sizeof ( value_type ) );
        tp.tag.ptr -= rhs * sizeof ( value_type );
        return *this;
    }

    [[maybe_unused]] constexpr tagged_pointer & operator++ ( ) noexcept {
        assert ( ( tp.tag.ptr - sizeof ( value_type ) ) <= ( std::numeric_limits<std::uintptr_t>::max ( ) >> 8 ) );
        tp.tag.ptr += sizeof ( value_type );
        return *this;
    }

    [[maybe_unused]] constexpr tagged_pointer operator++ ( int ) noexcept {
        const tagged_pointer tmp ( *this );
        operator++ ( );
        return tmp;
    }

    [[nodiscard]] constexpr std::ptrdiff_t operator+ ( tagged_pointer const & rhs ) const noexcept {
        assert ( ( tp.tag.ptr - rhs.tp.tag.ptr ) <= ( std::numeric_limits<std::uintptr_t>::max ( ) >> 8 ) );
        return static_cast<pointer> ( tp.tag.ptr ) + static_cast<pointer> ( rhs.tp.tag.ptr );
    }

    [[nodiscard]] constexpr std::ptrdiff_t operator+ ( pointer const rhs ) const noexcept {
        assert ( ( static_cast<pointer> ( tp.tag.ptr ) - rhs ) <= ( std::numeric_limits<std::uintptr_t>::max ( ) >> 8 ) );
        return static_cast<pointer> ( tp.tag.ptr ) + rhs;
    }

    // returns a pointer.
    template<typename Int, typename = std::enable_if_t<std::is_integral<Int>::value>>
    [[nodiscard]] constexpr pointer operator+ ( Int const rhs ) const noexcept {
        assert ( ( tp.tag.ptr - rhs * sizeof ( value_type ) ) <= ( std::numeric_limits<std::uintptr_t>::max ( ) >> 8 ) );
        return static_cast<pointer> ( tp.tag.ptr ) + rhs;
    }

    // return a tagged_pointer.
    template<typename Int, typename = std::enable_if_t<std::is_integral<Int>::value>>
    [[maybe_unused]] constexpr tagged_pointer & operator+= ( Int const rhs ) noexcept {
        assert ( ( tp.tag.ptr - rhs * sizeof ( value_type ) ) <= ( std::numeric_limits<std::uintptr_t>::max ( ) >> 8 ) );
        tp.tag.ptr += rhs * sizeof ( value_type );
        return *this;
    }

    [[nodiscard]] constexpr bool operator== ( tagged_pointer rhs ) const noexcept { return tp.ptr == rhs.tp.ptr; }

    [[nodiscard]] constexpr bool operator!= ( tagged_pointer rhs ) const noexcept { return tp.ptr != rhs.tp.ptr; }

    [[nodiscard]] constexpr bool operator< ( tagged_pointer rhs ) const noexcept {
        return static_cast<pointer> ( tp.tag.ptr ) < static_cast<pointer> ( rhs.tp.tag.ptr );
    }

    [[nodiscard]] constexpr bool operator> ( tagged_pointer rhs ) const noexcept {
        return static_cast<pointer> ( tp.tag.ptr ) > static_cast<pointer> ( rhs.tp.tag.ptr );
    }

    [[nodiscard]] constexpr bool operator<= ( tagged_pointer rhs ) const noexcept { return not( *this > rhs ); }

    [[nodiscard]] constexpr bool operator>= ( tagged_pointer rhs ) const noexcept { return not( *this < rhs ); }

    [[nodiscard]] friend constexpr value_type & operator* ( tagged_pointer tp ) noexcept {
        return *static_cast<pointer> ( tp.tp.tag.ptr );
    }

    pointer get ( ) const noexcept { return static_cast<pointer> ( tp.tag.ptr ); }
};
