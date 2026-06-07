#pragma once

#include <strong_type/strong_type.hpp>
#include <strong_type/convertible_to.hpp>
#include <strong_type/formattable.hpp>
#include <strong_type/ordered.hpp>
#include "xray/math/math_std.hpp"

namespace xray::math {

namespace details {

template<typename U, typename TagType>
    requires std::is_floating_point_v<U>
using StrongTypeBaseFPLike =
    strong::type<U, TagType, strong::semiregular, strong::formattable, strong::arithmetic, strong::partially_ordered>;

template<typename TagType>
using StrongTypeBaseF32Like = StrongTypeBaseFPLike<float, TagType>;

template<typename TagType>
using StrongTypeBaseF64Like = StrongTypeBaseFPLike<double, TagType>;

}

template<typename FPType>
    requires std::is_floating_point_v<FPType>
class Radians;

template<typename FPType>
    requires std::is_floating_point_v<FPType>
class Degrees : public details::StrongTypeBaseFPLike<FPType, struct DegreesTag>
{
    using BaseType = details::StrongTypeBaseFPLike<FPType, struct DegreesTag>;

  public:
    Degrees() = default;
    constexpr explicit Degrees(const FPType degrees) noexcept
        : BaseType{ degrees }
    {
    }

    inline explicit operator Radians<FPType>() const noexcept;
};

template<typename FPType>
    requires std::is_floating_point_v<FPType>
class Radians : public details::StrongTypeBaseFPLike<FPType, struct RadiansTag>
{
    using BaseType = details::StrongTypeBaseFPLike<FPType, struct RadiansTag>;

  public:
    Radians() = default;

    constexpr explicit Radians(const float rads) noexcept
        : BaseType{ rads }
    {
    }

    explicit operator Degrees<FPType>() const noexcept { return Degrees<FPType>{ math::degrees(this->value_of()) }; }
};

template<typename FPType>
    requires std::is_floating_point_v<FPType>
inline Degrees<FPType>::operator Radians<FPType>() const noexcept
{
    return Radians<FPType>{ math::radians(this->value_of()) };
}

using DegreesF32 = Degrees<float>;
using DegreesF64 = Degrees<double>;

using RadiansF32 = Radians<float>;
using RadiansF64 = Radians<double>;

}

inline namespace literals {
constexpr xray::math::RadiansF32
operator""_RADF32(long double rads)
{
    return xray::math::RadiansF32{ static_cast<float>(rads) };
}

constexpr xray::math::DegreesF32
operator""_DEGF32(long double degrees)
{
    return xray::math::DegreesF32{ static_cast<float>(degrees) };
}

constexpr xray::math::RadiansF32
operator""_DEG2RADF32(long double deg)
{
    return xray::math::RadiansF32{ xray::math::radians(static_cast<float>(deg)) };
}

constexpr xray::math::DegreesF32
operator""_RAD2DEGF32(long double rad)
{
    return xray::math::DegreesF32{ xray::math::degrees(static_cast<float>(rad)) };
}

}
