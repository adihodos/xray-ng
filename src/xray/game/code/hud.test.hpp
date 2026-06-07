#pragma once

#include "xray/xray.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <swl/variant.hpp>
#include <ankerl/unordered_dense.h>
#include "xray/scene/scene.description.hpp"

struct xrPoint3D {
	xray::F32 x{};
	xray::F32 y{};
	xray::F32 z{};
};

using MyVarType = swl::variant<std::string, xray::ISIZE, xrPoint3D>;

struct xrHudInfo_t {
	xray::F32 x{};
	xray::F32 pos3d[3]{};
	xray::ISIZE elements{};
	std::string name{};
	std::string scenes[4]{};
	std::string sval{};
	std::vector<xrPoint3D> pts{};
	ankerl::unordered_dense::map<xray::U32, xrPoint3D> coords{};
	std::unordered_map<std::string, xrPoint3D> coords2{};
	// swl::variant<std::string, xray::ISIZE, xrPoint3D> variant_val{};
	MyVarType variant_val{};
	swl::variant<xrPoint3D, xray::F32, xray::ISIZE> variant2{};
	xray::scene::GltfGeometryDescription gltf{ .name = "xfury", .path = "c:/temp/3d/sa23.gltf"};
};
