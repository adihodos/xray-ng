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

/// \file unique_pointer.hpp

#include "xray/xray.hpp"

#include <cassert>
#include <memory>
#include <new>
#include <tuple>
#include <type_traits>

#ifndef _LIBCPP_INLINE_VISIBILITY
#define _LIBCPP_INLINE_VISIBILITY
#endif

#ifndef _LIBCPP_CONSTEXPR
#define _LIBCPP_CONSTEXPR
#endif

#ifndef _LIBCPP_EXPLICIT
#define _LIBCPP_EXPLICIT explicit
#endif

#ifndef _NOEXCEPT
#define _NOEXCEPT noexcept
#endif

namespace xray {
namespace base {

/// \addtogroup __GroupXrayBase
/// @{

/// \brief  The following classes are adapted from Clang's libcxx

template<class _Tp>
struct __has_element_type
{
  private:
    struct __two
    {
        char __lx;
        char __lxx;
    };
    template<class _Up>
    static __two __test(...);
    template<class _Up>
    static char __test(typename _Up::element_type* = 0);

  public:
    static const bool value = sizeof(__test<_Tp>(0)) == 1;
};

namespace __has_pointer_type_imp {
struct __two
{
    char __lx;
    char __lxx;
};

template<class _Up>
static __two
__test(...);
template<class _Up>
static char
__test(typename _Up::pointer* = 0);
} // namespace __has_pointer_type_imp

template<class _Tp>
struct __has_pointer_type : public std::integral_constant<bool, sizeof(__has_pointer_type_imp::__test<_Tp>(0)) == 1>
{
};

namespace __pointer_type_imp {

template<class _Tp, class _Dp, bool = __has_pointer_type<_Dp>::value>
struct __pointer_type
{
    typedef typename _Dp::pointer type;
};

template<class _Tp, class _Dp>
struct __pointer_type<_Tp, _Dp, false>
{
    typedef _Tp* type;
};

} // namespace __pointer_type_imp

template<class _Tp, class _Dp>
struct __pointer_type
{
    typedef typename __pointer_type_imp::__pointer_type<_Tp, typename std::remove_reference<_Dp>::type>::type type;
};

template<class _T1,
         class _T2,
         bool = std::is_same<typename std::remove_cv<_T1>::type, typename std::remove_cv<_T2>::type>::value,
         bool = std::is_empty<_T1>::value,
         bool = std::is_empty<_T2>::value>

struct __libcpp_compressed_pair_switch;

template<class _T1, class _T2, bool IsSame>
struct __libcpp_compressed_pair_switch<_T1, _T2, IsSame, false, false>
{
    enum
    {
        value = 0
    };
};

template<class _T1, class _T2, bool IsSame>
struct __libcpp_compressed_pair_switch<_T1, _T2, IsSame, true, false>
{
    enum
    {
        value = 1
    };
};

template<class _T1, class _T2, bool IsSame>
struct __libcpp_compressed_pair_switch<_T1, _T2, IsSame, false, true>
{
    enum
    {
        value = 2
    };
};

template<class _T1, class _T2>
struct __libcpp_compressed_pair_switch<_T1, _T2, false, true, true>
{
    enum
    {
        value = 3
    };
};

template<class _T1, class _T2>
struct __libcpp_compressed_pair_switch<_T1, _T2, true, true, true>
{
    enum
    {
        value = 1
    };
};

template<class _T1, class _T2, unsigned = __libcpp_compressed_pair_switch<_T1, _T2>::value>
class __libcpp_compressed_pair_imp;

template<class _T1, class _T2>
class __libcpp_compressed_pair_imp<_T1, _T2, 0>
{
  private:
    _T1 __first_;
    _T2 __second_;

  public:
    typedef _T1 _T1_param;
    typedef _T2 _T2_param;

    typedef typename std::remove_reference<_T1>::type& _T1_reference;
    typedef typename std::remove_reference<_T2>::type& _T2_reference;

    typedef const typename std::remove_reference<_T1>::type& _T1_const_reference;
    typedef const typename std::remove_reference<_T2>::type& _T2_const_reference;

    _LIBCPP_INLINE_VISIBILITY __libcpp_compressed_pair_imp() {}
    _LIBCPP_INLINE_VISIBILITY explicit __libcpp_compressed_pair_imp(_T1_param __t1)
        : __first_(std::forward<_T1_param>(__t1))
    {
    }
    _LIBCPP_INLINE_VISIBILITY explicit __libcpp_compressed_pair_imp(_T2_param __t2)
        : __second_(std::forward<_T2_param>(__t2))
    {
    }
    _LIBCPP_INLINE_VISIBILITY __libcpp_compressed_pair_imp(_T1_param __t1, _T2_param __t2)
        : __first_(std::forward<_T1_param>(__t1))
        , __second_(std::forward<_T2_param>(__t2))
    {
    }

    _LIBCPP_INLINE_VISIBILITY _T1_reference first() _NOEXCEPT { return __first_; }
    _LIBCPP_INLINE_VISIBILITY _T1_const_reference first() const _NOEXCEPT { return __first_; }

    _LIBCPP_INLINE_VISIBILITY _T2_reference second() _NOEXCEPT { return __second_; }
    _LIBCPP_INLINE_VISIBILITY _T2_const_reference second() const _NOEXCEPT { return __second_; }

    _LIBCPP_INLINE_VISIBILITY void swap(__libcpp_compressed_pair_imp& __x) _NOEXCEPT
    {
        using std::swap;
        swap(__first_, __x.__first_);
        swap(__second_, __x.__second_);
    }
};

template<class _T1, class _T2>
class __libcpp_compressed_pair_imp<_T1, _T2, 1> : private _T1
{
  private:
    _T2 __second_;

  public:
    typedef _T1 _T1_param;
    typedef _T2 _T2_param;

    typedef _T1& _T1_reference;
    typedef typename std::remove_reference<_T2>::type& _T2_reference;

    typedef const _T1& _T1_const_reference;
    typedef const typename std::remove_reference<_T2>::type& _T2_const_reference;

    _LIBCPP_INLINE_VISIBILITY __libcpp_compressed_pair_imp() {}
    _LIBCPP_INLINE_VISIBILITY explicit __libcpp_compressed_pair_imp(_T1_param __t1)
        : _T1(std::forward<_T1_param>(__t1))
    {
    }
    _LIBCPP_INLINE_VISIBILITY explicit __libcpp_compressed_pair_imp(_T2_param __t2)
        : __second_(std::forward<_T2_param>(__t2))
    {
    }
    _LIBCPP_INLINE_VISIBILITY __libcpp_compressed_pair_imp(_T1_param __t1, _T2_param __t2)
        : _T1(std::forward<_T1_param>(__t1))
        , __second_(std::forward<_T2_param>(__t2))
    {
    }

    _LIBCPP_INLINE_VISIBILITY _T1_reference first() _NOEXCEPT { return *this; }
    _LIBCPP_INLINE_VISIBILITY _T1_const_reference first() const _NOEXCEPT { return *this; }

    _LIBCPP_INLINE_VISIBILITY _T2_reference second() _NOEXCEPT { return __second_; }
    _LIBCPP_INLINE_VISIBILITY _T2_const_reference second() const _NOEXCEPT { return __second_; }

    _LIBCPP_INLINE_VISIBILITY void swap(__libcpp_compressed_pair_imp& __x)
    {
        using std::swap;
        swap(__second_, __x.__second_);
    }
};

template<class _T1, class _T2>
class __libcpp_compressed_pair_imp<_T1, _T2, 2> : private _T2
{
  private:
    _T1 __first_;

  public:
    typedef _T1 _T1_param;
    typedef _T2 _T2_param;

    typedef typename std::remove_reference<_T1>::type& _T1_reference;
    typedef _T2& _T2_reference;

    typedef const typename std::remove_reference<_T1>::type& _T1_const_reference;
    typedef const _T2& _T2_const_reference;

    _LIBCPP_INLINE_VISIBILITY __libcpp_compressed_pair_imp() {}
    _LIBCPP_INLINE_VISIBILITY explicit __libcpp_compressed_pair_imp(_T1_param __t1)
        : __first_(std::forward<_T1_param>(__t1))
    {
    }
    _LIBCPP_INLINE_VISIBILITY explicit __libcpp_compressed_pair_imp(_T2_param __t2)
        : _T2(std::forward<_T2_param>(__t2))
    {
    }
    _LIBCPP_INLINE_VISIBILITY __libcpp_compressed_pair_imp(_T1_param __t1, _T2_param __t2)
        : _T2(std::forward<_T2_param>(__t2))
        , __first_(std::forward<_T1_param>(__t1))
    {
    }

    _LIBCPP_INLINE_VISIBILITY _T1_reference first() _NOEXCEPT { return __first_; }
    _LIBCPP_INLINE_VISIBILITY _T1_const_reference first() const _NOEXCEPT { return __first_; }

    _LIBCPP_INLINE_VISIBILITY _T2_reference second() _NOEXCEPT { return *this; }
    _LIBCPP_INLINE_VISIBILITY _T2_const_reference second() const _NOEXCEPT { return *this; }

    _LIBCPP_INLINE_VISIBILITY void swap(__libcpp_compressed_pair_imp& __x)
    {
        using std::swap;
        swap(__first_, __x.__first_);
    }
};

template<class _T1, class _T2>
class __libcpp_compressed_pair_imp<_T1, _T2, 3>
    : private _T1
    , private _T2
{
  public:
    typedef _T1 _T1_param;
    typedef _T2 _T2_param;

    typedef _T1& _T1_reference;
    typedef _T2& _T2_reference;

    typedef const _T1& _T1_const_reference;
    typedef const _T2& _T2_const_reference;

    _LIBCPP_INLINE_VISIBILITY __libcpp_compressed_pair_imp() {}
    _LIBCPP_INLINE_VISIBILITY explicit __libcpp_compressed_pair_imp(_T1_param __t1)
        : _T1(std::forward<_T1_param>(__t1))
    {
    }
    _LIBCPP_INLINE_VISIBILITY explicit __libcpp_compressed_pair_imp(_T2_param __t2)
        : _T2(std::forward<_T2_param>(__t2))
    {
    }
    _LIBCPP_INLINE_VISIBILITY __libcpp_compressed_pair_imp(_T1_param __t1, _T2_param __t2)
        : _T1(std::forward<_T1_param>(__t1))
        , _T2(std::forward<_T2_param>(__t2))
    {
    }

    _LIBCPP_INLINE_VISIBILITY _T1_reference first() _NOEXCEPT { return *this; }
    _LIBCPP_INLINE_VISIBILITY _T1_const_reference first() const _NOEXCEPT { return *this; }

    _LIBCPP_INLINE_VISIBILITY _T2_reference second() _NOEXCEPT { return *this; }
    _LIBCPP_INLINE_VISIBILITY _T2_const_reference second() const _NOEXCEPT { return *this; }

    _LIBCPP_INLINE_VISIBILITY void swap(__libcpp_compressed_pair_imp&) {}
};

template<class _T1, class _T2>
class __compressed_pair : private __libcpp_compressed_pair_imp<_T1, _T2>
{
    typedef __libcpp_compressed_pair_imp<_T1, _T2> base;

  public:
    typedef typename base::_T1_param _T1_param;
    typedef typename base::_T2_param _T2_param;

    typedef typename base::_T1_reference _T1_reference;
    typedef typename base::_T2_reference _T2_reference;

    typedef typename base::_T1_const_reference _T1_const_reference;
    typedef typename base::_T2_const_reference _T2_const_reference;

    _LIBCPP_INLINE_VISIBILITY __compressed_pair() {}
    _LIBCPP_INLINE_VISIBILITY explicit __compressed_pair(_T1_param __t1)
        : base(std::forward<_T1_param>(__t1))
    {
    }
    _LIBCPP_INLINE_VISIBILITY explicit __compressed_pair(_T2_param __t2)
        : base(std::forward<_T2_param>(__t2))
    {
    }
    _LIBCPP_INLINE_VISIBILITY __compressed_pair(_T1_param __t1, _T2_param __t2)
        : base(std::forward<_T1_param>(__t1), std::forward<_T2_param>(__t2))
    {
    }

    _LIBCPP_INLINE_VISIBILITY _T1_reference first() _NOEXCEPT { return base::first(); }
    _LIBCPP_INLINE_VISIBILITY _T1_const_reference first() const _NOEXCEPT { return base::first(); }

    _LIBCPP_INLINE_VISIBILITY _T2_reference second() _NOEXCEPT { return base::second(); }
    _LIBCPP_INLINE_VISIBILITY _T2_const_reference second() const _NOEXCEPT { return base::second(); }

    _LIBCPP_INLINE_VISIBILITY void swap(__compressed_pair& __x) { base::swap(__x); }
};

template<class _T1, class _T2>
inline _LIBCPP_INLINE_VISIBILITY void
swap(__compressed_pair<_T1, _T2>& __x, __compressed_pair<_T1, _T2>& __y)
{
    __x.swap(__y);
}

// __same_or_less_cv_qualified

template<class _Ptr1,
         class _Ptr2,
         bool = std::is_same<typename std::remove_cv<typename std::pointer_traits<_Ptr1>::element_type>::type,
                             typename std::remove_cv<typename std::pointer_traits<_Ptr2>::element_type>::type>::value>
struct __same_or_less_cv_qualified_imp : std::is_convertible<_Ptr1, _Ptr2>
{
};

template<class _Ptr1, class _Ptr2>
struct __same_or_less_cv_qualified_imp<_Ptr1, _Ptr2, false> : std::false_type
{
};

template<class _Ptr1,
         class _Ptr2,
         bool = std::is_pointer<_Ptr1>::value || std::is_same<_Ptr1, _Ptr2>::value || __has_element_type<_Ptr1>::value>
struct __same_or_less_cv_qualified : __same_or_less_cv_qualified_imp<_Ptr1, _Ptr2>
{
};

template<class _Ptr1, class _Ptr2>
struct __same_or_less_cv_qualified<_Ptr1, _Ptr2, false> : std::false_type
{
};

// default_delete

template<class _Tp>
struct default_delete
{
#ifndef _LIBCPP_HAS_NO_DEFAULTED_FUNCTIONS
    _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR default_delete() = default;
#else
    _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR default_delete() _NOEXCEPT {}
#endif
    template<class _Up>
    _LIBCPP_INLINE_VISIBILITY default_delete(
        const default_delete<_Up>&,
        typename std::enable_if<std::is_convertible<_Up*, _Tp*>::value>::type* = 0) _NOEXCEPT
    {
    }
    _LIBCPP_INLINE_VISIBILITY void operator()(_Tp* __ptr) const _NOEXCEPT
    {
        static_assert(sizeof(_Tp) > 0, "default_delete can not delete incomplete type");
        static_assert(!std::is_void<_Tp>::value, "default_delete can not delete incomplete type");
        delete __ptr;
    }
};

template<class _Tp>
struct default_delete<_Tp[]>
{
  public:
#ifndef _LIBCPP_HAS_NO_DEFAULTED_FUNCTIONS
    _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR default_delete() = default;
#else
    _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR default_delete() _NOEXCEPT {}
#endif
    template<class _Up>
    _LIBCPP_INLINE_VISIBILITY default_delete(
        const default_delete<_Up[]>&,
        typename std::enable_if<__same_or_less_cv_qualified<_Up*, _Tp*>::value>::type* = 0) _NOEXCEPT
    {
    }
    template<class _Up>
    _LIBCPP_INLINE_VISIBILITY void operator()(
        _Up* __ptr,
        typename std::enable_if<__same_or_less_cv_qualified<_Up*, _Tp*>::value>::type* = 0) const _NOEXCEPT
    {
        static_assert(sizeof(_Tp) > 0, "default_delete can not delete incomplete type");
        static_assert(!std::is_void<_Tp>::value, "default_delete can not delete incomplete type");
        delete[] __ptr;
    }
};

template<class _Tp, class _Dp = default_delete<_Tp>>
class unique_pointer
{
  public:
    typedef _Tp element_type;
    typedef _Dp deleter_type;
    typedef typename __pointer_type<_Tp, deleter_type>::type pointer;
    typedef unique_pointer<_Tp, _Dp> class_type;

  private:
    __compressed_pair<pointer, deleter_type> __ptr_;

    struct __nat
    {
        int __for_bool_;
    };

    typedef typename std::remove_reference<deleter_type>::type& _Dp_reference;
    typedef const typename std::remove_reference<deleter_type>::type& _Dp_const_reference;

  public:
    _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR unique_pointer() _NOEXCEPT : __ptr_(pointer())
    {
        static_assert(!std::is_pointer<deleter_type>::value,
                      "unique_pointer constructed with null function pointer deleter");
    }

    _LIBCPP_INLINE_VISIBILITY
    _LIBCPP_CONSTEXPR unique_pointer(std::nullptr_t) _NOEXCEPT : __ptr_(pointer())
    {
        static_assert(!std::is_pointer<deleter_type>::value,
                      "unique_pointer constructed with null function pointer deleter");
    }

    _LIBCPP_INLINE_VISIBILITY explicit unique_pointer(pointer __p) _NOEXCEPT : __ptr_(std::move(__p))
    {
        static_assert(!std::is_pointer<deleter_type>::value,
                      "unique_pointer constructed with null function pointer deleter");
    }

    _LIBCPP_INLINE_VISIBILITY unique_pointer(
        pointer __p,
        typename std::conditional<std::is_reference<deleter_type>::value,
                                  deleter_type,
                                  typename std::add_lvalue_reference<const deleter_type>::type>::type __d) _NOEXCEPT
        : __ptr_(__p, __d)
    {
    }

    _LIBCPP_INLINE_VISIBILITY unique_pointer(pointer __p,
                                             typename std::remove_reference<deleter_type>::type&& __d) _NOEXCEPT
        : __ptr_(__p, std::move(__d))
    {
        static_assert(!std::is_reference<deleter_type>::value, "rvalue deleter bound to reference");
    }

    _LIBCPP_INLINE_VISIBILITY unique_pointer(unique_pointer&& __u) _NOEXCEPT
        : __ptr_(unique_pointer_release(__u), std::forward<deleter_type>(unique_pointer_get_deleter(__u)))
    {
    }

    template<class _Up, class _Ep>
    _LIBCPP_INLINE_VISIBILITY unique_pointer(
        unique_pointer<_Up, _Ep>&& __u,
        typename std::enable_if<!std::is_array<_Up>::value &&
                                    std::is_convertible<typename unique_pointer<_Up, _Ep>::pointer, pointer>::value &&
                                    std::is_convertible<_Ep, deleter_type>::value &&
                                    (!std::is_reference<deleter_type>::value || std::is_same<deleter_type, _Ep>::value),
                                __nat>::type = __nat()) _NOEXCEPT
        : __ptr_(unique_pointer_release(__u), std::forward<_Ep>(unique_pointer_get_deleter(__u)))
    {
    }

    _LIBCPP_INLINE_VISIBILITY unique_pointer& operator=(unique_pointer&& __u) _NOEXCEPT
    {
        unique_pointer_reset(*this, unique_pointer_release(__u));
        __ptr_.second() = std::forward<deleter_type>(unique_pointer_get_deleter(__u));
        return *this;
    }

    template<class _Up, class _Ep>
    _LIBCPP_INLINE_VISIBILITY
        typename std::enable_if<!std::is_array<_Up>::value &&
                                    std::is_convertible<typename unique_pointer<_Up, _Ep>::pointer, pointer>::value &&
                                    std::is_assignable<deleter_type&, _Ep&&>::value,
                                unique_pointer&>::type
        operator=(unique_pointer<_Up, _Ep>&& __u) _NOEXCEPT
    {
        unique_pointer_reset(*this, unique_pointer_release(__u));
        __ptr_.second() = std::forward<_Ep>(unique_pointer_get_deleter(__u));
        return *this;
    }

    _LIBCPP_INLINE_VISIBILITY ~unique_pointer() { unique_pointer_reset(*this); }

    _LIBCPP_INLINE_VISIBILITY unique_pointer& operator=(std::nullptr_t) _NOEXCEPT
    {
        unique_pointer_reset(*this);
        return *this;
    }

    _LIBCPP_INLINE_VISIBILITY typename std::add_lvalue_reference<_Tp>::type operator*() const
    {
        return *__ptr_.first();
    }

    _LIBCPP_INLINE_VISIBILITY pointer operator->() const _NOEXCEPT { return __ptr_.first(); }

    template<typename _Tptr, typename _DelPtr>
    friend typename unique_pointer<_Tptr, _DelPtr>::pointer unique_pointer_get_ptr(
        const unique_pointer<_Tptr, _DelPtr>&) _NOEXCEPT;

    template<typename _Tptr, typename _DelPtr>
    friend typename unique_pointer<_Tptr, _DelPtr>::_Dp_reference unique_pointer_get_deleter(
        unique_pointer<_Tptr, _DelPtr>&) _NOEXCEPT;

    template<typename _Tptr, typename _DelPtr>
    friend typename unique_pointer<_Tptr, _DelPtr>::_Dp_const_reference unique_pointer_get_deleter(
        const unique_pointer<_Tptr, _DelPtr>&) _NOEXCEPT;

    template<typename _Tptr, typename _DelPtr>
    friend typename std::add_pointer<typename unique_pointer<_Tptr, _DelPtr>::pointer>::type unique_pointer_get_ptr_ptr(
        unique_pointer<_Tptr, _DelPtr>&) _NOEXCEPT;

    template<typename _Tptr, typename _DelPtr>
    friend typename unique_pointer<_Tptr, _DelPtr>::pointer unique_pointer_release(
        unique_pointer<_Tptr, _DelPtr>&) _NOEXCEPT;

    template<typename _Tptr, typename _DelPtr>
    friend void unique_pointer_reset(unique_pointer<_Tptr, _DelPtr>&,
                                     typename unique_pointer<_Tptr, _DelPtr>::pointer) _NOEXCEPT;

    template<typename _Tptr, typename _DelPtr>
    friend void unique_pointer_reset(unique_pointer<_Tptr, _DelPtr>&) _NOEXCEPT;

    template<typename _Tptr, typename _DelPtr>
    friend void unique_pointer_reset(unique_pointer<_Tptr, _DelPtr>&, std::nullptr_t) _NOEXCEPT;

    template<typename _Tptr, typename _Dptr>
    friend void swap(unique_pointer<_Tptr, _Dptr>&, unique_pointer<_Tptr, _Dptr>&) _NOEXCEPT;

  public:
    _LIBCPP_INLINE_VISIBILITY _LIBCPP_EXPLICIT operator bool() const _NOEXCEPT { return __ptr_.first() != nullptr; }
};

/// \brief  Specialisation of unique_pointer<> for arrays.
template<class _Tp, class _Dp>
class unique_pointer<_Tp[], _Dp>
{
  public:
    typedef _Tp element_type;
    typedef _Dp deleter_type;
    typedef typename __pointer_type<_Tp, deleter_type>::type pointer;
    typedef unique_pointer<_Tp[], _Dp> class_type;

  private:
    __compressed_pair<pointer, deleter_type> __ptr_;

    struct __nat
    {
        int __for_bool_;
    };

    typedef typename std::remove_reference<deleter_type>::type& _Dp_reference;
    typedef const typename std::remove_reference<deleter_type>::type& _Dp_const_reference;

  public:
    _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR unique_pointer() _NOEXCEPT : __ptr_(pointer())
    {
        static_assert(!std::is_pointer<deleter_type>::value,
                      "unique_pointer constructed with null function pointer deleter");
    }

    _LIBCPP_INLINE_VISIBILITY
    _LIBCPP_CONSTEXPR unique_pointer(std::nullptr_t) _NOEXCEPT : __ptr_(pointer())
    {
        static_assert(!std::is_pointer<deleter_type>::value,
                      "unique_pointer constructed with null function pointer deleter");
    }

    template<class _Pp>
    _LIBCPP_INLINE_VISIBILITY explicit unique_pointer(
        _Pp __p,
        typename std::enable_if<__same_or_less_cv_qualified<_Pp, pointer>::value, __nat>::type = __nat()) _NOEXCEPT
        : __ptr_(__p)
    {
        static_assert(!std::is_pointer<deleter_type>::value,
                      "unique_pointer constructed with null function pointer deleter");
    }

    template<class _Pp>
    _LIBCPP_INLINE_VISIBILITY unique_pointer(
        _Pp __p,
        typename std::conditional<std::is_reference<deleter_type>::value,
                                  deleter_type,
                                  typename std::add_lvalue_reference<const deleter_type>::type>::type __d,
        typename std::enable_if<__same_or_less_cv_qualified<_Pp, pointer>::value, __nat>::type = __nat()) _NOEXCEPT
        : __ptr_(__p, __d)
    {
    }

    _LIBCPP_INLINE_VISIBILITY unique_pointer(
        std::nullptr_t,
        typename std::conditional<std::is_reference<deleter_type>::value,
                                  deleter_type,
                                  typename std::add_lvalue_reference<const deleter_type>::type>::type __d) _NOEXCEPT
        : __ptr_(pointer(), __d)
    {
    }

    template<class _Pp>
    _LIBCPP_INLINE_VISIBILITY unique_pointer(
        _Pp __p,
        typename std::remove_reference<deleter_type>::type&& __d,
        typename std::enable_if<__same_or_less_cv_qualified<_Pp, pointer>::value, __nat>::type = __nat()) _NOEXCEPT
        : __ptr_(__p, std::move(__d))
    {
        static_assert(!std::is_reference<deleter_type>::value, "rvalue deleter bound to reference");
    }

    _LIBCPP_INLINE_VISIBILITY unique_pointer(std::nullptr_t,
                                             typename std::remove_reference<deleter_type>::type&& __d) _NOEXCEPT
        : __ptr_(pointer(), std::move(__d))
    {
        static_assert(!std::is_reference<deleter_type>::value, "rvalue deleter bound to reference");
    }

    _LIBCPP_INLINE_VISIBILITY unique_pointer(unique_pointer&& __u) _NOEXCEPT
        : __ptr_(unique_pointer_release(__u), std::forward<deleter_type>(unique_pointer_get_deleter(__u)))
    {
    }

    _LIBCPP_INLINE_VISIBILITY unique_pointer& operator=(unique_pointer&& __u) _NOEXCEPT
    {
        unique_pointer_reset(*this, unique_pointer_release(__u));
        __ptr_.second() = std::forward<deleter_type>(unique_pointer_get_deleter(__u));
        return *this;
    }

    template<class _Up, class _Ep>
    _LIBCPP_INLINE_VISIBILITY unique_pointer(
        unique_pointer<_Up, _Ep>&& __u,
        typename std::enable_if<
            std::is_array<_Up>::value &&
                __same_or_less_cv_qualified<typename unique_pointer<_Up, _Ep>::pointer, pointer>::value &&
                std::is_convertible<_Ep, deleter_type>::value &&
                (!std::is_reference<deleter_type>::value || std::is_same<deleter_type, _Ep>::value),
            __nat>::type = __nat()) _NOEXCEPT
        : __ptr_(unique_pointer_release(__u), std::forward<deleter_type>(unique_pointer_get_deleter(__u)))
    {
    }

    template<class _Up, class _Ep>
    _LIBCPP_INLINE_VISIBILITY typename std::enable_if<
        std::is_array<_Up>::value &&
            __same_or_less_cv_qualified<typename unique_pointer<_Up, _Ep>::pointer, pointer>::value &&
            std::is_assignable<deleter_type&, _Ep&&>::value,
        unique_pointer&>::type
    operator=(unique_pointer<_Up, _Ep>&& __u) _NOEXCEPT
    {
        unique_pointer_reset(*this, unique_pointer_release(__u));
        __ptr_.second() = std::forward<_Ep>(unique_pointer_get_deleter(__u));
        return *this;
    }

    _LIBCPP_INLINE_VISIBILITY ~unique_pointer() { unique_pointer_reset(*this); }

    _LIBCPP_INLINE_VISIBILITY unique_pointer& operator=(std::nullptr_t) _NOEXCEPT
    {
        unique_pointer_reset(*this);
        return *this;
    }

    _LIBCPP_INLINE_VISIBILITY typename std::add_lvalue_reference<_Tp>::type operator[](size_t __i) const
    {
        return __ptr_.first()[__i];
    }

    _LIBCPP_INLINE_VISIBILITY _LIBCPP_EXPLICIT operator bool() const _NOEXCEPT { return __ptr_.first() != nullptr; }

    template<typename _Tptr, typename _DelPtr>
    friend typename unique_pointer<_Tptr, _DelPtr>::pointer unique_pointer_get_ptr(
        const unique_pointer<_Tptr, _DelPtr>&) _NOEXCEPT;

    template<typename _Tptr, typename _DelPtr>
    friend typename unique_pointer<_Tptr, _DelPtr>::_Dp_reference unique_pointer_get_deleter(
        unique_pointer<_Tptr, _DelPtr>&) _NOEXCEPT;

    template<typename _Tptr, typename _DelPtr>
    friend typename unique_pointer<_Tptr, _DelPtr>::_Dp_const_reference unique_pointer_get_deleter(
        const unique_pointer<_Tptr, _DelPtr>&) _NOEXCEPT;

    template<typename _Tptr, typename _DelPtr>
    friend typename std::add_pointer<typename unique_pointer<_Tptr, _DelPtr>::pointer>::type unique_pointer_get_ptr_ptr(
        unique_pointer<_Tptr, _DelPtr>&) _NOEXCEPT;

    template<typename _Tptr, typename _DelPtr>
    friend typename unique_pointer<_Tptr, _DelPtr>::pointer unique_pointer_release(
        unique_pointer<_Tptr, _DelPtr>&) _NOEXCEPT;

    template<typename _Tptr, typename _DelPtr>
    friend void unique_pointer_reset(unique_pointer<_Tptr, _DelPtr>&,
                                     typename unique_pointer<_Tptr, _DelPtr>::pointer) _NOEXCEPT;

    template<typename _Tptr, typename _DelPtr>
    friend void unique_pointer_reset(unique_pointer<_Tptr, _DelPtr>&) _NOEXCEPT;

    template<typename _Tptr, typename _DelPtr>
    friend void unique_pointer_reset(unique_pointer<_Tptr, _DelPtr>&, std::nullptr_t) _NOEXCEPT;

    template<typename _Tptr, typename _Dptr>
    friend void swap(unique_pointer<_Tptr, _Dptr>&, unique_pointer<_Tptr, _Dptr>&) _NOEXCEPT;
};

/// \brief  Returns a reference to the deleter of the unique_pointer.
template<typename _Tptr, typename _DelPtr>
inline typename unique_pointer<_Tptr, _DelPtr>::_Dp_reference
unique_pointer_get_deleter(unique_pointer<_Tptr, _DelPtr>& __u) _NOEXCEPT
{
    return __u.__ptr_.second();
}

/// \brief  Returns a const reference to the deleter of the unique_pointer.
template<typename _Tptr, typename _DelPtr>
inline typename unique_pointer<_Tptr, _DelPtr>::_Dp_const_reference
unique_pointer_get_deleter(const unique_pointer<_Tptr, _DelPtr>& __u) _NOEXCEPT
{
    return __u.__ptr_.second();
}

/// \brief  Returns the owned pointer of a unique_pointer. The unique_pointer
///         is still the owner.
template<typename _Tptr, typename _DelPtr>
inline typename unique_pointer<_Tptr, _DelPtr>::pointer
unique_pointer_get_ptr(const unique_pointer<_Tptr, _DelPtr>& __u) _NOEXCEPT
{
    return __u.__ptr_.first();
}

/// \brief  Returns a pointer to the owned pointer. This us usefull when working
///         with COM-like apis for example. The unique_pointer object must not
///         own a pointer when this function is called.
/// \remarks    This function is potentially dangerous. In debug builds it
///             will assert if the unique_pointer already ownd another pointer.
template<typename _Tp, typename _Dp>
inline typename std::add_pointer<typename unique_pointer<_Tp, _Dp>::pointer>::type
unique_pointer_get_ptr_ptr(unique_pointer<_Tp, _Dp>& __u) _NOEXCEPT
{
    assert(!__u.__ptr_.first() && "Cannot own existing pointer if you want to use this!!");

    return &__u.__ptr_.first();
}

/// \brief  Releases ownership of the raw pointer to the user.
/// \remarks    It is the user's responsability to ensure the pointer's
///             destruction once it's no longer needed.
template<typename _Tp, typename _Dp>
inline typename unique_pointer<_Tp, _Dp>::pointer
unique_pointer_release(unique_pointer<_Tp, _Dp>& __u) _NOEXCEPT
{
    auto old_ptr = __u.__ptr_.first();
    __u.__ptr_.first() = typename unique_pointer<_Tp, _Dp>::pointer();
    return old_ptr;
}

/// \brief  Resets the raw pointer owned by the unique_pointer object.
/// \remarks    Self assign checking is only done in debug builds.
template<typename _Tptr, typename _DelPtr>
inline void
unique_pointer_reset(unique_pointer<_Tptr, _DelPtr>& __u,
                     typename unique_pointer<_Tptr, _DelPtr>::pointer __p) _NOEXCEPT
{
    // assert(__u.__ptr_.first() != __p);
    if (__u.__ptr_.first() == __p) {
        return;
    }

    auto __tmp = __u.__ptr_.first();
    __u.__ptr_.first() = __p;
    if (__tmp)
        __u.__ptr_.second()(__tmp);
}

/// \brief  Resets the raw pointer owned by the unique_pointer object to the
///         default value.
template<typename _Tptr, typename _DelPtr>
inline void
unique_pointer_reset(unique_pointer<_Tptr, _DelPtr>& __u) _NOEXCEPT
{
    auto __tmp = __u.__ptr_.first();
    __u.__ptr_.first() = typename unique_pointer<_Tptr, _DelPtr>::pointer();
    if (__tmp)
        __u.__ptr_.second()(__tmp);
}

/// \brief  Resets the raw pointer owned by the unique_pointer object to the
///         null pointer.
template<typename _Tptr, typename _DelPtr>
inline void
unique_pointer_reset(unique_pointer<_Tptr, _DelPtr>& __u, std::nullptr_t) _NOEXCEPT
{
    auto __tmp = __u.__ptr_.first();
    __u.__ptr_.first() = nullptr;
    if (__tmp)
        __u.__ptr_.second()(__tmp);
}

/// \brief  Swaps the contents of two unique_pointer objects.
template<typename _Tp, typename _Dp>
inline void
swap(unique_pointer<_Tp, _Dp>& left, unique_pointer<_Tp, _Dp>& right) _NOEXCEPT
{
    left.__ptr_.swap(right.__ptr_);
}

/// \brief  Tests if the pointer owned by the unique_pointer object is null.
template<typename _Tp, typename _Dp>
inline bool
operator==(const unique_pointer<_Tp, _Dp>& __u, std::nullptr_t) _NOEXCEPT
{
    return unique_pointer_get_ptr(__u) == nullptr;
}

/// \brief  Tests if the pointer owned by the unique_pointer object is null.
template<typename _Tp, typename _Dp>
inline bool
operator==(std::nullptr_t, const unique_pointer<_Tp, _Dp>& __u) _NOEXCEPT
{
    return unique_pointer_get_ptr(__u) == nullptr;
}

/// \brief  Tests if the pointer owned by the unique_pointer object is not null.
template<typename _Tp, typename _Dp>
inline bool
operator!=(const unique_pointer<_Tp, _Dp>& __u, std::nullptr_t) _NOEXCEPT
{
    return unique_pointer_get_ptr(__u) != nullptr;
}

/// \brief  Tests if the pointer owned by the unique_pointer object is not null.
template<typename _Tp, typename _Dp>
inline bool
operator!=(std::nullptr_t, const unique_pointer<_Tp, _Dp>& __u) _NOEXCEPT
{
    return unique_pointer_get_ptr(__u) != nullptr;
}

// template <typename _Tp, typename... _Args>
// unique_pointer<_Tp> make_unique(_Args... args) {
//  return unique_pointer<_Tp>(new _Tp{std::forward<_Args>(args)...});
//}
//
// template <typename _Tp>
// unique_pointer<_Tp[]> make_unique(const std::size_t count) {
//  return unique_pointer<_Tp[]>(new _Tp[count]());
//}

template<class _Tp>
struct __unique_if
{
    typedef unique_pointer<_Tp> __unique_single;
};

template<class _Tp>
struct __unique_if<_Tp[]>
{
    typedef unique_pointer<_Tp[]> __unique_array_unknown_bound;
};

template<class _Tp, size_t _Np>
struct __unique_if<_Tp[_Np]>
{
    typedef void __unique_array_known_bound;
};

template<class _Tp, class... _Args>
inline _LIBCPP_INLINE_VISIBILITY typename __unique_if<_Tp>::__unique_single
make_unique(_Args&&... __args)
{
    return unique_pointer<_Tp>(new _Tp(std::forward<_Args>(__args)...));
}

template<class _Tp>
inline _LIBCPP_INLINE_VISIBILITY typename __unique_if<_Tp>::__unique_array_unknown_bound
make_unique(size_t __n)
{
    typedef typename std::remove_extent<_Tp>::type _Up;
    return unique_pointer<_Tp>(new _Up[__n]());
}

template<class _Tp, class... _Args>
typename __unique_if<_Tp>::__unique_array_known_bound
make_unique(_Args&&...) = delete;

/// \brief  Accessor shim.
template<typename _Tptr, typename _DelPtr>
inline typename unique_pointer<_Tptr, _DelPtr>::pointer
raw_ptr(const unique_pointer<_Tptr, _DelPtr>& __u) _NOEXCEPT
{
    return unique_pointer_get_ptr(__u);
}

/// \brief Accessor shim
template<typename _Tp, typename _Dp>
inline typename std::add_pointer<typename unique_pointer<_Tp, _Dp>::pointer>::type
raw_ptr_ptr(unique_pointer<_Tp, _Dp>& __u) _NOEXCEPT
{
    return unique_pointer_get_ptr_ptr(__u);
}

/// @}

} // namespace base
} // namespace xray
