#pragma once

#include "xray/xray.hpp"
#include "xray/base/minstd/type.traits.query.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/quaternion.hpp"

namespace xray::math {

template <typename Real>
	requires xray::minstd::is_floating_point_v<Real>
struct Orientation {
	scalar3<Real> origin{scalar3<Real>::stdc::zero};
	quaternion<Real> rotation{quaternion<Real>::stdc::identity};
	Real scale{Real(1)};
};

using OrientationF32 = Orientation<xray::F32>;
using OrientationF64 = Orientation<xray::F64>;

}  // namespace xray::math
