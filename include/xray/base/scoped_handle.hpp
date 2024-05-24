//
// Copyright (c) 2011, 2012, 2013 Adrian Hodos
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//    * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//    * Neither the name of the author nor the
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

/// \file scoped_handle.hpp
/// \brief Automated resource management classes and utilities.

#include "xray/xray.hpp"

namespace xray {
namespace base {

/// \addtogroup __GroupXrayBase
/// {@

/// \brief   RAII class for different OS handles (socket descriptors,
///          file descriptors, etc...). Ownership is exclusive.
///          When going out of scope, the wrapped raw handle will be disposed
///          according to the policy specified at template instantiation.
/// \param   management_policy Defines traits for a raw handle and helper
///          functions.
template<typename management_policy>
class scoped_handle
{

    /// \name Typedefs
    /// @{

  public:
    ///< Handle policy type.
    typedef management_policy handle_policy;

    ///< Alias for the raw handle type.
    typedef typename handle_policy::handle_type handle_type;

    ///< Type of pointer to the raw handle type.
    typedef typename handle_policy::handle_pointer handle_pointer;

    ///< Type of reference to the raw handle type.
    typedef typename handle_policy::handle_reference handle_reference;

    ///< Type of constant reference to the raw handle type.
    typedef typename handle_policy::const_handle_reference const_handle_reference;

    ///< Fully qualified type of this class.
    typedef scoped_handle<management_policy> class_type;

    /// @}

    /// \name Constructors.
    ///  @{

  public:
    /// \brief Default constructor. Initializes object with a non valid handle of
    /// the corresponding type.
    scoped_handle()
        : handle_{ handle_policy::null_handle() }
    {
    }

    /// \brief   Initialize with an existing raw handle.
    ///          The object assumes ownership of the handle.
    explicit scoped_handle(handle_type new_handle)
        : handle_{ new_handle }
    {
    }

    /// \brief Construct from a temporary of the same type.
    scoped_handle(class_type&& right) { handle_ = right.release(); }

    ~scoped_handle() { handle_policy::dispose(handle_); }

    /// @}

    /// \name Sanity checking.
    ///  @{

  public:
    ///
    /// \brief   Supports sanity checking in the form of if(object) {}.
    explicit operator bool() const { return handle_ != handle_policy::null_handle(); }

    /// @}

  public:
    /// \name Non-member accessor and utility functions.
    ///  @{

    ///
    /// \brief Explicit access to the raw handle owned by this object.
    template<typename M>
    friend typename scoped_handle<M>::handle_type scoped_handle_get(const scoped_handle<M>&);

    ///
    /// \brief Release ownership of the native handle to the caller.
    template<typename M>
    friend typename scoped_handle<M>::handle_type scoped_handle_release(scoped_handle<M>&);

    ///
    /// \brief Reset the owned handle to a new value.
    /// \remarks The old handle is destroyed before the assignment.
    template<typename M>
    friend void scoped_handle_reset(scoped_handle<M>&, typename scoped_handle<M>::handle_type);

    ///
    /// \brief Returns a pointer to the raw handle.
    template<typename M>
    friend typename scoped_handle<M>::handle_pointer scoped_handle_get_impl(scoped_handle<M>& sh);

    ///
    /// \brief Swaps contents with another object of this type.
    template<typename M>
    friend void swap(scoped_handle<M>&, scoped_handle<M>&);

    /// @}

  public:
    /// \name Assignment operators.
    ///  @{

    ///
    /// \brief Assign operator from a temporary of the same type.
    class_type& operator=(class_type&& right)
    {
        swap(right);
        return *this;
    }

    ///
    /// \brief Assign from a raw handle and take ownership of it.
    class_type& operator=(handle_type new_handle)
    {
        if (new_handle != handle_) {
            handle_policy::dispose(handle_);
            handle_ = new_handle;
        }
        return *this;
    }

    /// @}

  private:
    handle_type handle_; ///< Owned handle.

  private:
    /// \name Internal helper functions.
    ///  @{

    handle_type get() const { return handle_; }

    handle_pointer get_impl_ptr() { return &handle_; }

    handle_type release()
    {
        handle_type temp_handle = handle_;
        handle_ = handle_policy::null_handle();
        return temp_handle;
    }

    void reset(handle_type new_handle)
    {
        if (handle_ != new_handle) {
            handle_policy::dispose(handle_);
            handle_ = new_handle;
        }
    }

    void swap(class_type& rhs)
    {
        handle_type tmp_handle = handle_;
        handle_ = rhs.release();
        rhs.handle_ = tmp_handle;
    }

  private:
    /// \name Disabled functions.
    ///  @{

    XRAY_NO_COPY(scoped_handle);
};

///
/// \brief Accessor funct
template<typename M>
inline typename scoped_handle<M>::handle_type
scoped_handle_get(const scoped_handle<M>& sh)
{
    return sh.get();
}

///
/// \brief   Releases ownership of the handle to the caller. The caller is
///          responsible for closing the handle when no longer nee
template<typename M>
inline typename scoped_handle<M>::handle_type
scoped_handle_release(scoped_handle<M>& sh)
{
    return sh.release();
}

///
/// \brief   Resets the owned handle to the specified one. The old handle is
///          released in the proc
template<typename M>
inline void
scoped_handle_reset(scoped_handle<M>& sh, typename scoped_handle<M>::handle_type new_value = M::null_handle())
{
    sh.reset(new_value);
}

///
/// \brief   Returns a pointer to the owned handle. The existing handle is
///          closed. This is a convenience function that exists because
///          of API's that return handles to the caller via pointers.
/// \code
///  v8::base::scoped_handle<my_handle_policy> h;
///  api_get_handle(scoped_handle_get_impl(h));
///  if (!h) {
///      ...
///  }
/// \endcode
template<typename M>
inline typename scoped_handle<M>::handle_pointer
scoped_handle_get_impl(scoped_handle<M>& sh)
{
    scoped_handle_reset(sh);
    return sh.get_impl_ptr();
}

///
/// \brief   Equality test between raw handle and scoped_handle obj
template<typename T>
inline bool
operator==(const typename T::handle_type& left, const scoped_handle<T>& right)
{
    return left == scoped_handle_get(right);
}

///
/// \brief   Equality test between raw handle and scoped_handle obj
template<typename T>
inline bool
operator==(const scoped_handle<T>& left, const typename T::handle_type& right)
{
    return right == left;
}

///
/// \brief   Inequality test between raw handle and scoped_handle obj
template<typename T>
inline bool
operator!=(const typename T::handle_type& left, const scoped_handle<T>& right)
{
    return !(left == right);
}

///
/// \brief   Inequality test between raw handle and scoped_handle obj
template<typename T>
inline bool
operator!=(const scoped_handle<T>& left, const typename T::handle_type& right)
{
    return right != left;
}

///
/// \brief   Swaps the owned handles of two scoped_handle obje
template<typename M>
inline void
swap(scoped_handle<M>& left, scoped_handle<M>& right)
{
    left.swap(right);
}

/// @}

} // namespace base
} // namespace v8
