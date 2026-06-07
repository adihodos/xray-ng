#pragma once

#include "xray/base/minstd/integral.constant.hpp"
#include "xray/base/minstd/declval.hpp"

namespace xray::minstd {

namespace detail {

//
// Not entirely correct, but good enough for my purpose
template <typename From, typename To>
class is_convertible_impl {
	template <typename T>
	static void convertible(T);

	template <typename F, typename = decltype(convertible<To>(declval<F>()))>
	static true_type test(int);

	template <typename>
	static false_type test(...);

public:
	using type = decltype(test<From>(0));
};

}  // namespace detail

template <typename From, typename To>
struct is_convertible : public detail::is_convertible_impl<From, To>::type {};

template <typename From, typename To>
inline constexpr bool is_convertible_v = is_convertible<From, To>::value;

}  // namespace xray::minstd
