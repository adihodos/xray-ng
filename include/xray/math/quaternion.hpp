//
// Copyright (c) 2011-2017 Adrian Hodos
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

/// \file   quaternion.hpp

#include <cmath>
#include <type_traits>

#include "xray/math/scalar3.hpp"
#include "xray/math/scalar3_math.hpp"
#include "xray/math/scalar3x3.hpp"

namespace xray {
namespace math {

/// \addtogroup __GroupXrayMath_Geometry
/// @{

/**
 * \brief Represents a quaternion, parameterized by the type of its components.
 * \remarks Quaternion multiplication is not commutative, that is given
 *          two quaternions \a q0 and \a q1, <b> q0 * q1 != q1 * q0 </b>.
 */
template<typename real_type>
    requires std::is_arithmetic_v<real_type>
class quaternion
{
  public:
    /** Type of components. */
    typedef real_type element_type;

    /** Type of reference to a component. */
    typedef real_type& reference;

    /** Type of const reference to a component. */
    typedef const real_type& const_reference;

    /** Fully qualified type of this class. */
    typedef quaternion<real_type> class_type;

    union
    {
        struct
        {
            real_type w;
            real_type x;
            real_type y;
            real_type z;
        };
        real_type components[4];
    };

    /** The null quaternion (identity for addition). */

    /** The identity quaternion for multiplication */

    /**
     \brief Default constructor, leaves the object uninitialized.
     */
    quaternion() noexcept = default;

    /**
     \brief Constructs a quaternion using the specified values.
     */
    inline constexpr quaternion(const real_type w, const real_type x, const real_type y, const real_type z) noexcept;

    template<typename U>
        requires std::is_convertible_v<U, real_type>
    constexpr quaternion(const U w, const U x, const U y, const U z) noexcept
        : quaternion{
            static_cast<real_type>(w),
            static_cast<real_type>(x),
            static_cast<real_type>(y),
            static_cast<real_type>(z),
        }
    {
    }

    constexpr quaternion(const real_type (&arr)[4]) noexcept
        : quaternion{ arr[3], arr[0], arr[1], arr[2] }
    {
    }

    template<typename U>
        requires std::is_convertible_v<U, real_type>
    constexpr quaternion(const U (&arr)[4]) noexcept
        : quaternion{
            static_cast<real_type>(arr[3]),
            static_cast<real_type>(arr[0]),
            static_cast<real_type>(arr[1]),
            static_cast<real_type>(arr[2]),
        }
    {
    }

    /**
     \brief Constructs a quaternion from axis-angle format.
     \param angle   The angle of rotation around the axis, expressed in radians.
     \param axis    Vector that represents the axis of rotation. It is not
                    necessary to be unit length.
     */
    inline quaternion(const real_type angle, const scalar3<real_type>& axis) noexcept;

    template<typename U>
        requires std::is_convertible_v<U, real_type>
    quaternion(const U angle, const scalar3<U>& axis) noexcept
        : quaternion{ static_cast<real_type>(angle), scalar3<real_type>{ axis } }
    {
    }

    /**
     \brief Given two vectors v1 and v2, constructs a quaternion that represents
            the rotation of v1 into v2.
     */
    quaternion(const scalar3<real_type>& v1, const scalar3<real_type>& v2) noexcept;

    template<typename U>
        requires std::is_convertible_v<U, real_type>
    quaternion(const scalar3<U>& v1, const scalar3<U>& v2) noexcept
        : quaternion{ scalar3<real_type>{ v1 }, scalar3<real_type>{ v2 } }
    {
    }

    /**
     \brief Converts the specified rotation matrix to quaternion format.
     \param[in] mtx Rotation matrix.
     */
    quaternion(const scalar3x3<real_type>& mtx) noexcept;

    /// \brief Self assign add.
    constexpr quaternion<real_type>& operator+=(const quaternion<real_type>& rhs) noexcept;

    /// \brief Self assing substract.
    constexpr quaternion<real_type>& operator-=(const quaternion<real_type>& rhs) noexcept;

    /// \brief Self multiply with scalar.
    constexpr quaternion<real_type>& operator*=(const real_type scalar) noexcept;

    /// \brief Self divide with scalar.
    constexpr quaternion<real_type>& operator/=(const real_type scalar) noexcept;

    struct stdc;
};

template<typename real_type>
    requires std::is_arithmetic_v<real_type>
inline constexpr quaternion<real_type>::quaternion(const real_type w_,
                                                   const real_type x_,
                                                   const real_type y_,
                                                   const real_type z_) noexcept
    : w(w_)
    , x(x_)
    , y(y_)
    , z(z_)
{
}

template<typename real_type>
    requires std::is_arithmetic_v<real_type>
inline quaternion<real_type>::quaternion(const real_type angle, const scalar3<real_type>& axis) noexcept
{
    const auto lensquared = length_squared(axis);

    if (is_zero(lensquared)) {
        w = real_type{ 1 };
        x = y = z = real_type{};
    } else {
        const real_type scale_factor = std::sin(angle / real_type{ 2 }) / std::sqrt(lensquared);

        w = std::cos(angle / real_type{ 2 });
        x = axis.x * scale_factor;
        y = axis.y * scale_factor;
        z = axis.z * scale_factor;
    }
}

template<typename real_type>
    requires std::is_arithmetic_v<real_type>
quaternion<real_type>::quaternion(const scalar3<real_type>& v1, const scalar3<real_type>& v2) noexcept
{
    //
    // What's the explanation behind this method ?? I can't remember where
    // I've seen it first.

    const auto bisector{ normalize(v1 + v2) };

    w = dot_product(v1, bisector);

    if (!is_zero(w)) {
        scalar3<real_type> axis(cross_product(v1, bisector));
        x = axis.x;
        y = axis.y;
        z = axis.z;
    } else {
        real_type inv_length;
        if (std::abs(v1.x) >= std::abs(v1.y)) {
            inv_length = math::inv_sqrt(v1.x * v1.x + v1.z * v1.z);
            x = -v1.z * inv_length;
            y = real_type{};
            z = +v1.x * inv_length;
        } else {
            inv_length = math::inv_sqrt(v1.y * v1.y + v1.z * v1.z);
            x = real_type{};
            y = +v1.z * inv_length;
            z = -v1.y * inv_length;
        }
    }
}

template<typename real_type>
    requires std::is_arithmetic_v<real_type>
constexpr quaternion<real_type>&
quaternion<real_type>::operator+=(const quaternion<real_type>& rhs) noexcept
{
    w += rhs.w;
    x += rhs.x;
    y += rhs.y;
    z += rhs.z;
    return *this;
}

template<typename real_type>
    requires std::is_arithmetic_v<real_type>
constexpr quaternion<real_type>&
quaternion<real_type>::operator-=(const quaternion<real_type>& rhs) noexcept
{
    w -= rhs.w;
    x -= rhs.x;
    y -= rhs.y;
    z -= rhs.z;
    return *this;
}

template<typename real_type>
    requires std::is_arithmetic_v<real_type>
constexpr quaternion<real_type>&
quaternion<real_type>::operator*=(const real_type scalar) noexcept
{
    w *= scalar;
    x *= scalar;
    y *= scalar;
    z *= scalar;
    return *this;
}

template<typename real_type>
    requires std::is_arithmetic_v<real_type>
constexpr quaternion<real_type>&
quaternion<real_type>::operator/=(const real_type scalar) noexcept
{
    return *this *= (real_type{ 1 } / scalar);
}

template<typename T>
    requires std::is_arithmetic_v<T>
struct quaternion<T>::stdc
{
    static constexpr const quaternion<T> zero{ T{}, T{}, T{}, T{} };
    static constexpr const quaternion<T> identity{ T{ 1 }, T{}, T{}, T{} };
};

template<typename T>
    requires std::is_arithmetic_v<T>
constexpr const quaternion<T> quaternion<T>::stdc::zero;

template<typename T>
    requires std::is_arithmetic_v<T>
constexpr const quaternion<T> quaternion<T>::stdc::identity;

using quaternionf = quaternion<float>;
using quaterniond = quaternion<double>;

using quatf = quaternion<float>;
using quatd = quaternion<double>;

/// @}

} // namespace math
} // namespace xray
