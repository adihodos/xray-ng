#pragma once

#include <type_traits>
#include "xray/math/scalar3.hpp"
#include "xray/math/quaternion.hpp"

namespace xray::math {

template<typename Real>
    requires std::is_floating_point_v<Real>
struct Orientation
{
    scalar3<Real> origin{ scalar3<Real>::stdc::zero };
    quaternion<Real> rotation{ quaternion<Real>::stdc::identity };
    Real scale{ Real(1) };
};

using OrientationF32 = Orientation<float>;
using OrientationF64 = Orientation<double>;

}
