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
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR THE CONTRIBUTORS BE LIABLE FOR
// ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

///
/// \file unique_handle.hpp

#pragma once

#include "xray/xray.hpp"
#include <cassert>

namespace xray {
namespace base {

template <typename H>
class unique_handle;

/// \brief      Returns the handle owned by a unique_handle object.
template <typename H>
typename H::handle_type
unique_handle_get(const unique_handle<H>& holder) noexcept;

/// \brief      Releases ownership of the handle to the caller.
template <typename H>
inline typename H::handle_type
unique_handle_release(unique_handle<H>&) noexcept;

/// \brief      Resets the handle owned to the the specified one. The old handle
///             is destroyed.
template <typename H>
inline void
unique_handle_reset(unique_handle<H>&,
                    typename H::handle_type new_handle = H::null()) noexcept;

/// \brief      Returns a pointer to the owned handle. The object must not own a
///             a valid handle before this call is made.
template <typename H>
inline typename H::handle_type* unique_handle_ptr(unique_handle<H>&) noexcept;

/// \class      unique_handle
///
/// \brief      Wraps a handle to a resource, with exclusive ownership
/// semantics.
///             When this object goes out of scope, the resource represented by
///             the handle will be deallocated.
///
/// \tparam     HType  A class that defines the characteristic of a handle to
///             some resource. This class must provide the following members :
///             - handle_type - a typedef to the actual handle
///                             (socket, file descriptor, etc)
///             - null - static member function that returns the invalid value
///                      for that particular handle (nullptr for a FILE* handle,
///                      -1 for a socket handle, etc)
///             - is_null - static member function that returns true if the
///                       handle's value is equal to the null handle
///             - destroy - static member function that closes the handle
///
/// \code
///
/// struct gl_buffer_handle {
///      using handle_type = GLuint;
///
///      static constexpr handle_type null() noexcept {
///          return 0;
///      }
///
///      static bool is_null(const handle_type h) noexcept {
///          return h == 0;
///      }
///
///      static void destroy(const handle_type h) noexcept {
///          glDeleteBuffers(1, &h);
///      }
/// };
///
/// void example_func() {
///      unique_handle<gl_buffer_handle> scoped_vb;
///      glGenBuffers(1, raw_handle_ptr(scoped_vb));
///
///      if (!scoped_vb) {
///          //
///          // Failed to create buffer, handle error.
///          return;
///      }
///
///      glBindBuffer(GL_ARRAY_BUFFER, raw_handle(scoped_vb));
///      glBufferData(GL_ARRAY_BUFFER, ...);
///
///      //
///      //  When control flow exits the function, the buffer handle will be
///      //  automatically released.
/// }
///
/// \endcode
template <typename HType>
class unique_handle {

  /// \name Defined types
  /// @{

public:
  using handle_type = typename HType::handle_type;

  /// @}

  /// \name Construction/destruction
  /// @{

public:
  /// \brief      Default constructor, handle is null (invalid).
  unique_handle() noexcept : handle_{HType::null()} {}

  /// \brief      Construct by taking ownership of an existing handle.
  explicit unique_handle(const handle_type& raw_handle) noexcept
      : handle_{raw_handle} {}

  ~unique_handle() noexcept { HType::destroy(handle_); }

  /// @}

  /// \name Special constructors/operators
  /// @{

public:
  unique_handle(const unique_handle<HType>&) = delete;
  unique_handle<HType>& operator=(const unique_handle<HType>&) = delete;

  /// \brief      Construct from a temporary object of the same type.
  unique_handle(unique_handle<HType>&& rhs) noexcept : handle_{rhs.handle_} {
    rhs.handle_ = HType::null();
  }

  /// \brief      Assign from a temporary object of the same type. The
  ///             previously owned handle will be destroyed.
  unique_handle<HType>& operator=(unique_handle<HType>&& rhs) noexcept {
    if (this != &rhs) {
      HType::destroy(handle_);
      handle_     = rhs.handle_;
      rhs.handle_ = HType::null();
    }

    return *this;
  }

  /// \brief      Assigns a new handle for ownership.
  ///             The previously owned handle is destroyed.
  unique_handle<HType>& operator=(const handle_type& new_handle) noexcept {
    if (handle_ != new_handle) {
      HType::destroy(handle_);
      handle_ = new_handle;
    }

    return *this;
  }

  /// @}

  /// \name Sanity checking
  /// @{

  /// \brief      Test if handle is valid.
  explicit operator bool() const noexcept { return !HType::is_null(handle_); }

  /// @}

  /// \name   Friend accessor and helper functions.
  /// @{

public:
  template <typename H>
  friend typename H::handle_type
  unique_handle_get(const unique_handle<H>& holder) noexcept;

  template <typename H>
  friend typename H::handle_type
  unique_handle_release(unique_handle<H>&) noexcept;

  template <typename H>
  friend void unique_handle_reset(unique_handle<H>&,
                                  typename H::handle_type) noexcept;

  template <typename H>
  friend typename H::handle_type* unique_handle_ptr(unique_handle<H>&) noexcept;

  /// @}

  /// \name Private data members.
  /// @{
private:
  handle_type handle_; ///<   Owned handle to some resource.

  /// @}
};

template <typename HType>
inline typename HType::handle_type
unique_handle_get(const unique_handle<HType>& holder) noexcept {
  return holder.handle_;
}

template <typename HType>
inline typename HType::handle_type
unique_handle_release(unique_handle<HType>& holder) noexcept {
  auto owned_handle = holder.handle_;
  holder.handle_    = HType::null();
  return owned_handle;
}

template <typename HType>
inline void
unique_handle_reset(unique_handle<HType>&       holder,
                    typename HType::handle_type new_handle) noexcept {
  HType::destroy(holder.handle_);
  holder.handle_ = new_handle;
}

template <typename HType>
inline typename HType::handle_type*
unique_handle_ptr(unique_handle<HType>& holder) noexcept {
  assert(!holder);
  return &holder.handle_;
}

template <typename H>
inline bool operator==(const unique_handle<H>& lhs,
                       const unique_handle<H>& rhs) noexcept {
  return raw_handle(lhs) == raw_handle(rhs);
}

template <typename H>
inline bool operator!=(const unique_handle<H>& lhs,
                       const unique_handle<H>& rhs) noexcept {
  return !(lhs == rhs);
}

/// \brief      Accessor shim for unique_handle objects. Returns the owned
/// handle.
template <typename HType>
inline typename HType::handle_type
raw_handle(const unique_handle<HType>& holder) noexcept {
  return unique_handle_get(holder);
}

/// \brief      Accessor shim to get a pointer to the handle of a unique_handle
///             object.
template <typename HType>
inline typename HType::handle_type*
raw_handle_ptr(unique_handle<HType>& holder) noexcept {
  return unique_handle_ptr(holder);
}

} // namespace base
} // namespace xray
