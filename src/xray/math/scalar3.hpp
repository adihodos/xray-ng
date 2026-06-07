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

#include "xray/xray.hpp"
#include "xray/xray_types.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/base/minstd/type.traits.query.hpp"
#include "xray/base/minstd/convertible.hpp"
#include "xray/base/serialization/serialize.as.hpp"
#include "xray/base/xray.debug.hpp"
#include "xray/math/swizzle.hpp"
#include "xray/math/rank.hpp"

namespace xray {
namespace math {

/// \addtogroup __GroupXrayMath
/// @{

template <typename T>
	requires minstd::is_arithmetic_v<T>

struct[[= xray::base::serialize_as_array{}]] scalar3 : public SwizzleBase<T, 3> {
	union {
		struct {
			T x;
			T y;
			T z;
		};

		struct {
			T u;
			T v;
			T w;
		};

		struct {
			T s;
			T t;
			T p;
		};

		T components[3];
	};

	using class_type = scalar3<T>;

	scalar3() noexcept = default;

	explicit constexpr scalar3(const T val) noexcept : scalar3<T>{val, val, val} {}

	template <typename U>
		requires minstd::is_convertible_v<U, T>
	explicit constexpr scalar3(const U val) noexcept : scalar3{val, val, val} {}

	constexpr scalar3(const T xval, const T yval, const T zval) noexcept : x{xval}, y{yval}, z{zval} {}

	template <typename U>
		requires minstd::is_convertible_v<U, T>
	constexpr scalar3(const U x, const U y, const U z) noexcept {
		this->x = static_cast<U>(x);
		this->y = static_cast<U>(y);
		this->z = static_cast<U>(z);
	}

	// explicit constexpr scalar3(const std::array<T, 3>& arr) noexcept : scalar3{arr[0], arr[1], arr[2]} {}

	// template <typename U>
	// requires minstd::is_convertible_v<U, T>
	// explicit scalar3(const std::array<U, 3>& arr) noexcept : scalar3{arr[0], arr[1], arr[2]} {}

	explicit constexpr scalar3(const T (&arr)[3]) noexcept : scalar3{arr[0], arr[1], arr[2]} {}

	template <typename U>
		requires minstd::is_convertible_v<U, T>
	explicit scalar3(const U (&arr)[3]) noexcept : scalar3{arr[0], arr[1], arr[2]} {}

	constexpr scalar3(const Swizzle3<T> s) noexcept : scalar3{s.x, s.y, s.z} {}

	template <typename U>
		requires minstd::is_convertible_v<U, T>
	constexpr scalar3(const Swizzle3<U> s) noexcept : scalar3{s.x, s.y, s.z} {}

	/// \name Self assign math operators
	/// @{

	inline scalar3<T>& operator+=(const scalar3<T>& rhs) noexcept;
	inline scalar3<T>& operator-=(const scalar3<T>& rhs) noexcept;
	inline scalar3<T>& operator*=(const T scalar) noexcept;
	inline scalar3<T>& operator/=(const T scalar) noexcept;

	/// @}

	template <typename... Ts>
	constexpr auto swizzle() const noexcept {
		return SwizzleBase<T, 3>::swizzle(this->components, Ts{}...);
	}

public:
	/// \name Iterators and data access
	/// @{

	using value_type	 = T;
	using reference_type = T&;
	using iterator		 = T*;
	using const_iterator = const T*;

	constexpr T* data() noexcept { return components; }
	constexpr const T* data() const noexcept { return components; }
	constexpr const T* cdata() const noexcept { return components; }

	constexpr ISIZE size() const noexcept { return base::xr_array_size(components); }

	constexpr iterator begin() noexcept { return data(); }
	constexpr iterator end() noexcept { return data() + size(); }

	constexpr const_iterator begin() const noexcept { return data(); }
	constexpr const_iterator end() const noexcept { return data() + size(); }

	constexpr const_iterator cbegin() noexcept { return cdata(); }
	constexpr const_iterator cend() noexcept { return cdata() + size(); }

	constexpr const_iterator cbegin() const noexcept { return cdata(); }
	constexpr const_iterator cend() const noexcept { return cdata() + size(); }

	/// @}

	/// \name Standard constants for R3 vectors.
	/// @{

public:
	struct stdc;

	/// @}
};

template <typename T>
	requires minstd::is_arithmetic_v<T>
struct scalar3<T>::stdc {
	static constexpr const scalar3<T> unit_x{T(1.0), T(0.0), T(0.0)};
	static constexpr const scalar3<T> unit_y{T(0.0), T(1.0), T(0.0)};
	static constexpr const scalar3<T> unit_z{T(0.0), T(0.0), T(1.0)};
	static constexpr const scalar3<T> zero{T(0.0), T(0.0), T(0.0)};
	static constexpr const scalar3<T> one{T(1.0), T(1.0), T(1.0)};
};

template <typename T>
	requires minstd::is_arithmetic_v<T>
constexpr const scalar3<T> scalar3<T>::stdc::unit_x;

template <typename T>
	requires minstd::is_arithmetic_v<T>
constexpr const scalar3<T> scalar3<T>::stdc::unit_y;

template <typename T>
	requires minstd::is_arithmetic_v<T>
constexpr const scalar3<T> scalar3<T>::stdc::unit_z;

template <typename T>
	requires minstd::is_arithmetic_v<T>
constexpr const scalar3<T> scalar3<T>::stdc::zero;

template <typename T>
	requires minstd::is_arithmetic_v<T>
constexpr const scalar3<T> scalar3<T>::stdc::one;

using vec3f	   = scalar3<scalar_lowp>;
using vec3f32  = scalar3<scalar_lowp>;
using vec3d	   = scalar3<scalar_mediump>;
using vec3i8   = scalar3<I8>;
using vec3ui8  = scalar3<U8>;
using vec3i32  = scalar3<I32>;
using vec3ui32 = scalar3<U32>;

/// @}

}  // namespace math
}  // namespace xray
