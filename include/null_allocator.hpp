
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


#include <memory>


namespace pdr::detail {

template<typename T>
class null_allocator {

    public:

    using value_type = T;
    using pointer = value_type *;
    using const_pointer = typename std::pointer_traits<pointer>::template rebind<const value_type>;

    template<typename U>
    struct rebind {
        using other = null_allocator<U>;
    };

    null_allocator ( ) noexcept { }
    template<typename U>
    null_allocator ( null_allocator<U> const & ) noexcept { }

    constexpr pointer allocate ( std::size_t ) const noexcept { return nullptr; }
    constexpr void deallocate ( pointer, std::size_t ) const noexcept { }

    using propagate_on_container_copy_assignment = std::false_type;
    using propagate_on_container_move_assignment = std::false_type;
    using propagate_on_container_swap = std::false_type;
    using is_always_equal = std::is_empty<null_allocator>;
};

}

namespace pdr {

template<typename T, typename U>
bool operator == ( const detail::null_allocator<T> &, const detail::null_allocator<U> & ) noexcept {
    return true;
}

template<typename T, typename U>
bool operator != ( const detail::null_allocator<T> &, const detail::null_allocator<U> & ) noexcept {
    return false;
}

}
