//
// Copyright (c) 2011, 2012, 2013 Adrian Hodos
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the author nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR THE CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <cassert>
#include <initializer_list>
#include <iterator>
#include <type_traits>

#include "xray/xray.hpp"

namespace xray {
namespace base {

///
/// \brief A fixed size, POD only, stack instantiated std::vector like class.
template<typename T, size_t k_vec_dimension = 64U>
class fixed_pod_vector
{

    /// \name Defined types.
    /// @{

  public:
    typedef T value_type;
    typedef T& reference;
    typedef const T& const_reference;

    typedef T* pointer;
    typedef const T* const_pointer;

    typedef T* iterator;
    typedef const T* const_iterator;

    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    typedef ptrdiff_t difference_type;
    typedef size_t size_type;

    typedef fixed_pod_vector<T, k_vec_dimension> class_type;

    /// @}

    /// \name Constructors.
    /// @{

  public:
    static_assert(std::is_pod<T>::value, "POD-only container!");

    fixed_pod_vector() noexcept
        : vec_size_{ 0 }
    {
    }

    fixed_pod_vector(const class_type& rhs) noexcept
    {
        memcpy(data(), rhs.data(), rhs.byte_count());
        vec_size_ = rhs.size();
    }

    fixed_pod_vector(const_pointer first, const_pointer last)
    {
        assert((last - first) < k_vec_dimension);
        vec_size_ = last - first;
        memcpy(data(), first, byte_count());
    }

    fixed_pod_vector(std::initializer_list<T> init_lst) noexcept
        : fixed_pod_vector{ init_lst.begin(), init_lst.end() }
    {
    }

    /// @}

    /// \name Operators.
    /// @{

  public:
    class_type& operator=(const class_type& rhs) noexcept
    {
        if (this != &rhs) {
            memcpy(data(), rhs.data(), rhs.size() * sizeof(T));
            vec_size_ = rhs.size();
        }

        return *this;
    }

    reference operator[](size_t idx) noexcept
    {
        assert(idx < size());
        return vec_data_[idx];
    }

    const_reference operator[](size_t idx) const noexcept
    {
        assert(idx < size());
        return vec_data_[idx];
    }

    /// @}

    /// \name Attributes.
    /// @{

  public:
    bool empty() const noexcept { return size() == 0; }

    size_t size() const noexcept { return vec_size_; }

    size_t max_size() const noexcept { return k_vec_dimension; }

    /// @}

    /// \name Element access.
    /// @{

  public:
    reference front() noexcept
    {
        assert(!empty());
        return vec_data_[0];
    }

    const_reference front() const noexcept
    {
        assert(!empty());
        return vec_data_[0];
    }

    reference back() noexcept
    {
        assert(!empty());
        return vec_data_[size() - 1];
    }

    const_reference back() const noexcept
    {
        assert(!empty());
        return vec_data_[size() - 1];
    }

    pointer data() noexcept { return &vec_data_[0]; }

    const_pointer data() const noexcept { return &vec_data_[0]; }

    /// @}

    /// \name Iteration.
    /// @{

  public:
    iterator begin() noexcept { return &vec_data_[0]; }

    const_iterator begin() const noexcept { return &vec_data_[0]; }

    iterator end() noexcept { return &vec_data_[size()]; }

    const_iterator end() const noexcept { return &vec_data_[size()]; }

    reverse_iterator rbegin() noexcept { return reverse_iterator(begin()); }

    const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(begin()); }

    reverse_iterator rend() noexcept { return reverse_iterator(end()); }

    const_reverse_iterator rend() const noexcept { return const_reverse_iterator(end()); }

    /// @}

    /// \name Operations.
    /// @{

    void clear() noexcept { vec_size_ = 0; }

    void push_back(const T& element) noexcept
    {
        assert(size() < max_size());
        vec_data_[vec_size_++] = element;
    }

    void pop_back() noexcept
    {
        assert(size() > 0);
        --vec_size_;
    }

    void resize(const size_type new_size) noexcept
    {
        assert(new_size < max_size());
        vec_size_ = new_size;
    }

    void swap(class_type& rhs) noexcept
    {
        T temp_buff[k_vec_dimension];

        memcpy(temp_buff, &rhs[0], rhs.byte_count());
        memcpy(&rhs[0], vec_data_, byte_count());
        memcpy(vec_data_, &temp_buff[0], rhs.byte_count());
        std::swap(vec_size_, rhs.vec_size_);
    }

    /// @}

    /// \name Internal Helper functions.
    /// @{

  private:
    size_t byte_count() const noexcept { return vec_size_ * sizeof(T); }

    /// @}

    /// \name Data members.
    /// @{

  private:
    T vec_data_[k_vec_dimension];
    size_type vec_size_;

    /// @}
};

} // namespace base
} // namespace xray
