#pragma once

#include "xray/xray.hpp"
#include "xray/base/xray.slice.hpp"
#include "xray/base/xray.stringview.hpp"

namespace xray::base {

struct MemoryArena;

enum class xrSerializeResult_t {
	Success,
	Error,
};

//
// serialization
template <typename T>
xrSerializeResult_t serialize_to_file(const T& value, const xray::base::xrStringView_t file);
template <typename T>
bool serialize_to_memory(const T& data, xrSlice_t<U8> mem);

//
// deserialization
template <typename T>
bool deserialize(xray::base::MemoryArena& arena, const C8* str, T& data);
	
template <typename T>
bool deserialize_from_file(xray::base::MemoryArena& arena, T& data, const xrStringView_t file);

}  // namespace xray::base
