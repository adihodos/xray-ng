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

#pragma once

/// \file xray.hpp
/// \brief Root header for the xray library.
/// Identifies the compiler and the operating system when
/// building the library. Also contains typedefs for the integer types.

#include <cstddef>
#include <cstdint>

#define XRAY_FUNCTION_NAME __FUNCTION__
#define XRAY_QUALIFIED_FUNCTION_NAME __PRETTY_FUNCTION__

#define XRAY_STRINGIZE_a(x) #x
#define XRAY_STRINGIZE_w(x) L## #x
#define XRAY_PASTE_X_Y(X, Y) X##Y
#define XRAY_PASTE_X_Y_Z(X, Y, Z) X##Y##Z

#define XRAY_STRING_WIDEN(x) XRAY_STRINGIZE_w(x)

#define XRAY_CONCATENATE(s1, s2) XRAY_PASTE_X_Y(s1, s2)

#ifdef __COUNTER__
#define XRAY_ANONYMOUS_VARIABLE(str) XRAY_CONCATENATE(str, __COUNTER__)
#else
#define XRAY_ANONYMOUS_VARIABLE(str) XRAY_CONCATENATE(str, __LINE__)
#endif

#define XRAY_STRING_WIDEN(x) XRAY_STRINGIZE_w(x)

#ifndef __WFILE__
#define __WFILE__ XRAY_STRING_WIDEN(__FILE__)
#endif

#if defined(XRAY_COMPILER_IS_GCC)
#undef XRAY_COMPILER_IS_GCC
#endif

#if defined(XRAY_COMPILER_IS_MINGW)
#undef XRAY_COMPILER_IS_MINGW
#endif

#if defined(XRAY_COMPILER_IS_CLANG)
#undef XRAY_COMPILER_IS_CLANG
#endif

#if defined(XRAY_COMPILER_IS_MSVC)
#undef XRAY_COMPILER_IS_MSVC
#endif

///
/// Compiler detection macros.
/// See : http://sourceforge.net/p/predef/wiki/Compilers/

#if defined(_MSC_VER)

#define XRAY_COMPILER_IS_MSVC
#define XRAY_COMPILER_STRING "Microsoft Visual C++ Compiler"

#elif defined(__GNUC__) && !defined(__clang__)

#define XRAY_COMPILER_IS_GCC
#define XRAY_COMPILER_STRING "GNU C/C++ Compiler"

#elif defined(__MINGW32__)

#define XRAY_COMPILER_IS_MINGW
#define XRAY_COMPILER_STRING "MinGW Toolchain"

#elif defined(__GNUC__) && defined(__clang__)

/// clang defines both __GNUC__ and __clang__
#define XRAY_COMPILER_IS_CLANG
#define XRAY_COMPILER_STRING "Clang C/C++ Compiler"

#else

#error Unsupported compiler.

#endif

///
/// OS detection macros.
/// See : http://sourceforge.net/p/predef/wiki/OperatingSystems/

#if defined(XRAY_OS_IS_WINDOWS)
#undef XRAY_OS_IS_WINDOWS
#endif

#if defined(XRAY_OS_IS_POSIX_COMPLIANT)
#undef XRAY_OS_IS_POSIX_COMPLIANT
#endif

#if defined(XRAY_OS_IS_POSIX_FAMILY)
#undef XRAY_OS_IS_POSIX_FAMILY
#endif

#if defined(XRAY_OS_IS_LINUX)
#undef XRAY_OS_IS_LINUX
#endif

#if defined(XRAY_OS_IS_FREEBSD)
#undef XRAY_OS_IS_FREEBSD
#endif

#if defined(__FreeBSD__)

#define XRAY_OS_IS_POSIX_COMPLIANT
#define XRAY_OS_IS_POSIX_FAMILY
#define XRAY_OS_IS_FREEBSD
#define XRAY_OS_STRING "FreeBSD"

#elif defined(__gnu_linux__)

#define XRAY_OS_IS_POSIX_COMPLIANT
#define XRAY_OS_IS_POSIX_FAMILY
#define XRAY_OS_IS_LINUX
#define XRAY_OS_STRING "GNU Linux"

#elif defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__WINDOWS__)

#define XRAY_OS_IS_WINDOWS
#define XRAY_OS_STRING "Windows"

#else

#error Unsupported system

#endif

/// \def XRAY_NO_COPY(type_name)
/// \brief   A macro to suppress automatic generation by the compiler of
///          the copy constructor and assignment operator. If not using ISO C++
///          11
///          compliant compiler, place it in the private part of the class
///          definition.
/// \code
///  class U {
/// private :
///      XRAY_NO_COPY(U);
///      ...
///  };
/// \endcode

#define XRAY_NO_COPY(xr_user_defined_type_name)                                                                        \
    xr_user_defined_type_name(xr_user_defined_type_name const&) = delete;                                              \
    xr_user_defined_type_name& operator=(xr_user_defined_type_name const&) = delete

#define XRAY_DEFAULT_MOVE(xr_user_defined_type_name)                                                                   \
    xr_user_defined_type_name(xr_user_defined_type_name&&) = default;                                                  \
    xr_user_defined_type_name& operator=(xr_user_defined_type_name&&) = default

///
/// \def XRAY_GEN_OPAQUE_TYPE(type)
/// \brief Generates a unique type.
#define XRAY_GEN_OPAQUE_TYPE(type)                                                                                     \
    typedef struct XRAY_internal_opaque_type##type                                                                     \
    {                                                                                                                  \
        int32_t i;                                                                                                     \
    } const* type;

#if defined(XRAY_COMPILER_IS_MSVC)

#ifndef SUPPRESS_WARNING_START
#define SUPPRESS_WARNING_START(args) __pragma(warning(push)) __pragma(warning(disable : args))
#endif /* !SUPPRESS_WARNING */

#ifndef SUPPRESS_WARNING_END
#define SUPPRESS_WARNING_END() __pragma(warning(pop))
#endif /* !SUPPRESS_WARNING_END */

#else /* XRAY_COMPILER_IS_MSVC */

#ifndef SUPPRESS_WARNING_START
#define SUPPRESS_WARNING_START(...)
#endif /* !SUPPRESS_WARNING */

#ifndef SUPPRESS_WARNING_END
#define SUPPRESS_WARNING_END()
#endif /* !SUPPRESS_WARNING_END */

#endif /* !XRAY_COMPILER_IS_MSVC */

#define XR_U32_OFFSETOF(type_name, member_name) static_cast<uint32_t>(offsetof(type_name, member_name))

#define XR_UNUSED_ARG(arg_name) (void)arg_name

namespace xray {
namespace base {

template<typename ContainerType, typename SizeType = std::size_t>
inline auto
container_bytes_size(const ContainerType& c) noexcept
{
    return static_cast<SizeType>(c.size() * sizeof(c[0]));
}

} // namespace base
} // namespace xray
