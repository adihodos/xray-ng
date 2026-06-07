#pragma once

#include "xray/xray.hpp"
#include "xray/math/scalar3.hpp"

namespace xray::rendering {

enum class ShapeKind : U8 {
	Disc,
	Square,
	Triangle,
	Diamond,
	Chevron,
	Ring,
	Tag,
	Cross,
	Asterisk,
	Infinity,
	BlockArrow,
};

struct alignas(16) ShapeSetup {
	xray::math::vec3f32 position;
	F32 size;
	F32 cos_theta;
	F32 sin_theta;
	F32 line_width;
	F32 antialias;
	U32 fg_color;
	U32 bg_color;
	U32 shape_kind;
};

}  // namespace xray::rendering
