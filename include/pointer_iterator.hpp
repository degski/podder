
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

#include <cstddef>

#include <sax/iostream.hpp>
#include <iterator>
#include <type_traits>


template<typename T, typename SizeType = std::size_t>
class const_pointer_iterator;

template<typename T, typename SizeType = std::size_t>
class pointer_iterator {

    friend class const_pointer_iterator<T, SizeType>;

    static_assert ( std::disjunction<std::is_same<SizeType, std::uint32_t>, std::is_same<SizeType, std::uint64_t>>::value, "SizeType must be an unsigned 32- or 64-bit integer type!" );

    public:

    using size_type = SizeType;
    using signed_size_type = typename std::conditional<std::is_same<SizeType, std::uint32_t>::value, std::int32_t, std::int64_t>::type;
    using difference_type = signed_size_type;
    using value_type = T;
    using reference = T &;
    using pointer = T *;
    using iterator_category = std::random_access_iterator_tag;

    pointer_iterator ( pointer p_ ) noexcept : m_pointer ( p_ ) { }
    pointer_iterator ( pointer_iterator const & pi_ ) noexcept : m_pointer ( pi_.m_pointer ) { }
    pointer_iterator ( pointer_iterator && pi_ ) noexcept : m_pointer ( std::forward<pointer> ( pi_.m_pointer ) ) { }

    [[ maybe_unused ]] pointer_iterator & operator ++ ( ) noexcept { ++m_pointer; return *this; }
    [[ maybe_unused ]] pointer_iterator operator ++ ( int ) noexcept { pointer_iterator it ( *this ); ++( *this ); return it; }

    [[ maybe_unused ]] pointer_iterator & operator -- ( ) noexcept { --m_pointer; return *this; }
    [[ maybe_unused ]] pointer_iterator operator -- ( int ) noexcept { pointer_iterator it ( *this ); --( *this ); return it; }

    [[ nodiscard ]] bool operator == ( pointer_iterator& other_ ) const noexcept { return m_pointer == other_.m_pointer; }
    [[ nodiscard ]] bool operator != ( pointer_iterator& other_ ) const noexcept { return not ( *this == other_ ); }

    [[ nodiscard ]] pointer_iterator operator + ( const size_type i_ ) const noexcept { return { m_pointer + i_ }; }
    [[ nodiscard ]] pointer_iterator operator - ( const size_type i_ ) const noexcept { return { m_pointer - i_ }; }

    [[ nodiscard ]] value_type& operator * ( ) noexcept { return *m_pointer; }
    [[ nodiscard ]] value_type& operator * ( ) const noexcept { return *m_pointer; }

    [[ nodiscard ]] pointer get ( ) const noexcept { return m_pointer; }

    private:

    pointer m_pointer;
};


template<typename T, typename SizeType>
class const_pointer_iterator {

    static_assert ( std::disjunction<std::is_same<SizeType, std::uint32_t>, std::is_same<SizeType, std::uint64_t>>::value, "SizeType must be an unsigned 32- or 64-bit integer type!" );

    public:

    using size_type = SizeType;
    using signed_size_type = typename std::conditional<std::is_same<SizeType, std::uint32_t>::value, std::int32_t, std::int64_t>::type;
    using difference_type = signed_size_type;
    using value_type = T;
    using const_reference = T const &;
    using const_pointer = T const *;
    using pointer = T *;
    using pointer_iterator = pointer_iterator<T, SizeType>;
    using iterator_category = std::random_access_iterator_tag;

    const_pointer_iterator ( const_pointer p_ ) noexcept : m_pointer ( p_ ) { }
    const_pointer_iterator ( pointer p_ ) noexcept : m_pointer ( const_cast<const_pointer> ( p_ ) ) { }
    const_pointer_iterator ( const_pointer_iterator const & pi_ ) noexcept : m_pointer ( pi_.m_pointer ) { }
    const_pointer_iterator ( const_pointer_iterator && pi_ ) noexcept : m_pointer ( std::forward<const_pointer> ( pi_.m_pointer ) ) { }
    const_pointer_iterator ( pointer_iterator && pi_ ) noexcept : m_pointer ( std::forward<const_pointer> ( pi_.m_pointer ) ) { }

    [[ maybe_unused ]] const_pointer_iterator & operator ++ ( ) noexcept { ++m_pointer; return *this; }
    [[ maybe_unused ]] const_pointer_iterator operator ++ ( int ) noexcept { const_pointer_iterator it ( *this ); ++( *this ); return it; }

    [[ maybe_unused ]] const_pointer_iterator& operator -- ( ) noexcept { --m_pointer; return *this; }
    [[ maybe_unused ]] const_pointer_iterator operator -- ( int ) noexcept { const_pointer_iterator it ( *this ); --( *this ); return it; }

    [[ nodiscard ]] bool operator == ( const_pointer_iterator & other_ ) const noexcept { return m_pointer == other_.m_pointer; }
    [[ nodiscard ]] bool operator != ( const_pointer_iterator & other_ ) const noexcept { return not ( *this == other_ ); }

    [[ nodiscard ]] const_pointer_iterator operator + ( const size_type i_ ) const noexcept { return { m_pointer + i_ }; }
    [[ nodiscard ]] const_pointer_iterator operator - ( const size_type i_ ) const noexcept { return { m_pointer - i_ }; }

    [[ nodiscard ]] value_type & operator * ( ) noexcept { return *m_pointer; }
    [[ nodiscard ]] value_type & operator * ( ) const noexcept { return *m_pointer; }

    [[ nodiscard ]] pointer get ( ) noexcept { return const_cast<pointer> ( m_pointer ); }
    [[ nodiscard ]] const_pointer get ( ) const noexcept { return m_pointer; }

    private:

    const_pointer m_pointer;
};


template<typename PointerIteratorType>
[[ nodiscard ]] typename PointerIteratorType::size_type operator - ( const typename PointerIteratorType::pointer p_, const PointerIteratorType it_ ) noexcept {
    return ( typename PointerIteratorType::size_type ) ( p_ - it_.get ( ) );
}

template<typename PointerIteratorType>
[[ nodiscard ]] typename PointerIteratorType::size_type operator - ( const PointerIteratorType it_, const typename PointerIteratorType::pointer p_ ) noexcept {
    return ( typename PointerIteratorType::size_type ) ( it_.get ( ) - p_ );
}

template<typename PointerIteratorType>
[[ nodiscard ]] typename PointerIteratorType::size_type operator - ( const PointerIteratorType it1_, const PointerIteratorType it2_ ) noexcept {
    return ( typename PointerIteratorType::size_type ) ( it1_.get ( ) - it2_.get ( ) );
}


template<typename T, typename SizeType = std::size_t>
class const_pointer_reverse_iterator;

template<typename T, typename SizeType = std::size_t>
class pointer_reverse_iterator {

    friend class const_pointer_reverse_iterator<T, SizeType>;

    static_assert ( std::disjunction<std::is_same<SizeType, std::uint32_t>, std::is_same<SizeType, std::uint64_t>>::value, "SizeType must be an unsigned 32- or 64-bit integer type!" );

    public:

    using size_type = SizeType;
    using signed_size_type = typename std::conditional<std::is_same<SizeType, std::uint32_t>::value, std::int32_t, std::int64_t>::type;
    using difference_type = signed_size_type;
    using value_type = T;
    using reference = T &;
    using pointer = T *;
    using iterator_category = std::random_access_iterator_tag;

    pointer_reverse_iterator ( pointer p_ ) noexcept : m_pointer ( p_ ) { }
    pointer_reverse_iterator ( pointer_reverse_iterator && pi_ ) noexcept : m_pointer ( std::forward<pointer> ( pi_.m_pointer ) ) { }

    [[ maybe_unused ]] pointer_reverse_iterator & operator -- ( ) noexcept { ++m_pointer; return *this; }
    [[ maybe_unused ]] pointer_reverse_iterator operator -- ( int ) noexcept { pointer_reverse_iterator it ( *this ); ++( *this ); return it; }

    [[ maybe_unused ]] pointer_reverse_iterator & operator ++ ( ) noexcept { --m_pointer; return *this; }
    [[ maybe_unused ]] pointer_reverse_iterator operator ++ ( int ) noexcept { pointer_reverse_iterator it ( *this ); --( *this ); return it; }

    [[ nodiscard ]] bool operator == ( pointer_reverse_iterator & other_ ) const noexcept { return m_pointer == other_.m_pointer; }
    [[ nodiscard ]] bool operator != ( pointer_reverse_iterator & other_ ) const noexcept { return not ( *this == other_ ); }

    [[ nodiscard ]] pointer_reverse_iterator operator - ( const size_type i_ ) const noexcept { return { m_pointer + i_ }; }
    [[ nodiscard ]] pointer_reverse_iterator operator + ( const size_type i_ ) const noexcept { return { m_pointer - i_ }; }

    [[ nodiscard ]] value_type & operator * ( ) noexcept { return *m_pointer; }
    [[ nodiscard ]] value_type & operator * ( ) const noexcept { return *m_pointer; }

    [[ nodiscard ]] pointer get ( ) const noexcept { return m_pointer; }

    private:

    pointer m_pointer;
};

template<typename T, typename SizeType>
class const_pointer_reverse_iterator {

    static_assert ( std::disjunction<std::is_same<SizeType, std::uint32_t>, std::is_same<SizeType, std::uint64_t>>::value, "SizeType must be an unsigned 32- or 64-bit integer type!" );

    public:

    using size_type = SizeType;
    using signed_size_type = typename std::conditional<std::is_same<SizeType, std::uint32_t>::value, std::int32_t, std::int64_t>::type;
    using difference_type = signed_size_type;
    using value_type = T;
    using const_reference = T const &;
    using const_pointer = T const *;
    using pointer = T *;
    using pointer_reverse_iterator = pointer_reverse_iterator<T, SizeType>;
    using iterator_category = std::random_access_iterator_tag;

    const_pointer_reverse_iterator ( const_pointer p_ ) noexcept : m_pointer ( p_ ) { }
    const_pointer_reverse_iterator ( pointer p_ ) noexcept : m_pointer ( const_cast<const_pointer> ( p_ ) ) { }
    const_pointer_reverse_iterator ( const_pointer_reverse_iterator && pi_ ) noexcept : m_pointer ( std::forward<const_pointer> ( pi_.m_pointer ) ) { }
    const_pointer_reverse_iterator ( pointer_reverse_iterator && pi_ ) noexcept : m_pointer ( std::forward<const_pointer> ( pi_.m_pointer ) ) { }

    [[ maybe_unused ]] const_pointer_reverse_iterator& operator -- ( ) noexcept { ++m_pointer; return *this; }
    [[ maybe_unused ]] const_pointer_reverse_iterator operator -- ( int ) noexcept { const_pointer_reverse_iterator it ( *this ); ++( *this ); return it; }

    [[ maybe_unused ]] const_pointer_reverse_iterator& operator ++ ( ) noexcept { --m_pointer; return *this; }
    [[ maybe_unused ]] const_pointer_reverse_iterator operator ++ ( int ) noexcept { const_pointer_reverse_iterator it ( *this ); --( *this ); return it; }

    [[ nodiscard ]] bool operator == ( const_pointer_reverse_iterator& other_ ) const noexcept { return m_pointer == other_.m_pointer; }
    [[ nodiscard ]] bool operator != ( const_pointer_reverse_iterator& other_ ) const noexcept { return not ( *this == other_ ); }

    [[ nodiscard ]] const_pointer_reverse_iterator operator - ( const size_type i_ ) const noexcept { return { m_pointer + i_ }; }
    [[ nodiscard ]] const_pointer_reverse_iterator operator + ( const size_type i_ ) const noexcept { return { m_pointer - i_ }; }

    [[ nodiscard ]] value_type& operator * ( ) noexcept { return *m_pointer; }
    [[ nodiscard ]] value_type& operator * ( ) const noexcept { return *m_pointer; }

    [[ nodiscard ]] const_pointer get ( ) const noexcept { return m_pointer; }

    private:

    const_pointer m_pointer;
};
