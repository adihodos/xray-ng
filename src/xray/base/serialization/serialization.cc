#include "xray/base/serialization/serialization.hpp"

#include <cstdio>
#include <meta>
#include <vector>
#include <string>
#include <string_view>
#include <filesystem>

#include <libconfig/libconfig.h>
#include <swl/variant.hpp>

#include "xray/base/xray.debug.hpp"
#include "xray/base/serialization/serialize.as.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/memory.arena.hpp"
#include "xray/base/thread.local.context.hpp"
#include "xray/base/scoped_guard.hpp"
#include "xray/base/xray.slice.hpp"
#include "xray/base/xray.string.hpp"
#include "xray/base/xray.stringview.hpp"
#include "xray/base/minstd/algo.hpp"
#include "xray/base/minstd/fwd.move.hpp"
#include "xray/base/minstd/remove.hpp"
#include "xray/base/minstd/type.traits.query.hpp"
#include "xray/base/minstd/voidt.hpp"

namespace stdm = std::meta;

//
// TODO: if tag union members are not tagged, it results in crazy compile errors
// maybe add a static assert for that case ...

//
// Document links
// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3394r4.html
// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p2996r13.html

namespace {

struct xrFileMemStream_t {
	static xrFileMemStream_t create(xray::base::xrSlice_t<xray::U8> mem) {
		FILE* fp = fmemopen(mem.s_ptr, (size_t)mem.s_len, "wt");
		if (!fp) {
			return {};
		}

		setvbuf(fp, nullptr, _IONBF, 0);

		return xrFileMemStream_t{
			.mem_ = mem,
			.fp_  = fp,
		};
	}

	static xrFileMemStream_t create(xray::base::MemoryArena& arena, const xray::ISIZE bytes) {
		using namespace xray;
		using namespace xray::base;
		xrSlice_t<U8> slice = arena.alloc_align(bytes, 4);
		if (!slice) {
			return xrFileMemStream_t{};
		}
		return create(slice);
	}

	void destroy() {
		if (fp_) {
			fclose(fp_);
		}
	}

	explicit operator bool() const noexcept { return fp_ != nullptr; }

	xray::base::xrSlice_t<xray::U8> mem_;
	FILE* fp_{nullptr};
};

template <typename T>
struct is_slice_type_t : xray::minstd::false_type {};

template <typename T>
struct is_slice_type_t<xray::base::xrSlice_t<T>> : xray::minstd::true_type {};

template <typename T>
inline constexpr bool is_slice_v = is_slice_type_t<xray::minstd::remove_const_t<T>>::value;

template <typename T, typename U>
concept same_as_type = xray::minstd::is_same_v<T, U> && xray::minstd::is_same_v<U, T>;

template <typename C>
concept SequenceContainerLike = requires(C cont) {
	typename C::iterator;
	typename C::const_iterator;
	typename C::value_type;

	{ cont.cbegin() } -> same_as_type<typename C::const_iterator>;
	{ cont.cend() } -> same_as_type<typename C::const_iterator>;
	{ cont.data() } -> same_as_type<typename C::value_type*>;
	{ cont[0] };
};

template <typename C>
concept AssociativeContainerLike = requires(C cont) {
	typename C::key_type;
	typename C::mapped_type;
	typename C::value_type;
	typename C::iterator;
	typename C::const_iterator;

	{ cont.cbegin() } -> same_as_type<typename C::const_iterator>;
	{ cont.cend() } -> same_as_type<typename C::const_iterator>;
	{ cont[typename C::key_type{}] } -> same_as_type<typename C::mapped_type&>;
};

template <typename T>
concept xrTaggedUnionLike = requires(T) {
	typename T::xrTagType_t;

	{ stdm::is_enum_type(^^typename T::xrTagType_t) };
};

template <typename T>
void serialize_one_element(config_setting_t* parent, const char* name, const T& value) {
	using ValueType			 = std::remove_cvref_t<std::decay_t<decltype(value)>>;
	const bool is_array_elem = config_setting_is_array(parent);

	if constexpr (std::is_integral_v<ValueType>) {
		if constexpr (sizeof(T) < sizeof(xray::I64)) {
			if (!is_array_elem) {
				config_setting_t* e = config_setting_add(parent, name, CONFIG_TYPE_INT);
				config_setting_set_int(e, value);
			} else {
				config_setting_set_int_elem(parent, -1, value);
			}
		} else {
			if (!is_array_elem) {
				config_setting_t* e = config_setting_add(parent, name, CONFIG_TYPE_INT64);
				config_setting_set_int64(e, value);
			} else {
				config_setting_set_int64_elem(parent, -1, value);
			}
		}
	} else if constexpr (xray::minstd::is_floating_point_v<ValueType>) {
		if (!is_array_elem) {
			config_setting_t* e = config_setting_add(parent, name, CONFIG_TYPE_FLOAT);
			config_setting_set_float(e, value);
		} else {
			config_setting_set_float_elem(parent, -1, value);
		}
	} else if constexpr (xray::minstd::is_same_v<xray::base::xrSlice_t<char>, ValueType> ||
						 xray::minstd::is_same_v<xray::base::xrSlice_t<const char>, ValueType>) {
		if (!is_array_elem) {
			config_setting_t* e = config_setting_add(parent, name, CONFIG_TYPE_STRING);
			config_setting_set_string_with_len(e, value.s_ptr, static_cast<size_t>(value.s_len));
		} else {
			config_setting_set_string_elem_with_len(parent, -1, value.s_ptr, value.s_len);
		}
	} else {
		static_assert(xray::minstd::dependant_always_false_v<ValueType>, "Unhandled for serialization");
	}
}

template <typename T>
void serialize_object(config_setting_t* parent, const char* name, const T& value);

template <typename T>
void serialize_array(config_setting_t* parent, const char* name, const xray::base::xrSlice_t<T> s) {
	constexpr stdm::info type_info = stdm::remove_cvref(^^T);

	constexpr auto arr_root_type = stdm::is_scalar_type(type_info) ? CONFIG_TYPE_ARRAY : CONFIG_TYPE_LIST;
	config_setting_t* arr_root	 = config_setting_add(parent, name, arr_root_type);
	for (xray::ISIZE idx = 0; idx < s.s_len; ++idx) {
		serialize_object(arr_root, nullptr, s[idx]);
	}
}

template <typename T>
	requires AssociativeContainerLike<T>
void serialize_associative_container(config_setting_t* root, const char* name, const T& table) {
	config_setting_t* obj_root = config_setting_add(root, name, CONFIG_TYPE_LIST);

	for (const auto& kvp : table) {
		serialize_object(obj_root, nullptr, kvp);
	}
}

template <typename T>
struct DisplayType {};

template <typename T>
void serialize_object(config_setting_t* parent, const char* name, const T& value) {
	constexpr stdm::access_context acc_ctx = stdm::access_context::unchecked();
	constexpr stdm::info type_info		   = stdm::dealias(stdm::remove_cvref(^^T));

	// XR_LOG_INFO_FILE_LINE("Serialize %s", stdm::display_string_of(type_info).data());

	template for ([[maybe_unused]] constexpr stdm::info annotation : std::define_static_array(
					  stdm::annotations_of_with_type(type_info, ^^xray::base::serialize_as_array)
				  )) {
		// XR_LOG_INFO(
		// 	"annotation: %s, type = %s, serialize type %s",
		// 	stdm::display_string_of(annotation).data(),
		// 	stdm::display_string_of(stdm::remove_cvref(stdm::type_of(annotation))).data(),
		// 	stdm::display_string_of(^^xray::base::serialize_as_array).data()
		// );

		serialize_array(
			parent, name, xray::base::slice_from_ptr_and_len(value.cdata(), static_cast<xray::ISIZE>(value.size()))
		);
		return;
	}

	if constexpr (stdm::is_same_type(type_info, ^^std::string) || stdm::is_same_type(type_info, ^^std::string_view) ||
				  stdm::is_same_type(type_info, ^^xray::base::xrString_t)) {
		serialize_one_element(
			parent, name, xray::base::slice_from_ptr_and_len(value.data(), static_cast<xray::ISIZE>(value.size()))
		);
	} else if constexpr (stdm::is_same_type(type_info, ^^std::filesystem::path)) {
		const std::string p = value.string();
		serialize_one_element(
			parent, name, xray::base::slice_from_ptr_and_len(p.c_str(), static_cast<xray::ISIZE>(p.size()))
		);
	} else if constexpr (SequenceContainerLike<typename[:type_info:]>) {
		serialize_array(
			parent, name, xray::base::slice_from_ptr_and_len(value.data(), static_cast<xray::ISIZE>(value.size()))
		);
	} else if constexpr (AssociativeContainerLike<typename[:type_info:]>) {
		serialize_associative_container(parent, name, value);
	} else if constexpr (xrTaggedUnionLike<typename[:type_info:]>) {
		XR_LOG_INFO("Tagged union like: %s", stdm::display_string_of(type_info).data());

		constexpr stdm::access_context acc_ctx = stdm::access_context::current();

		constexpr auto members = std::define_static_array(stdm::nonstatic_data_members_of(type_info, acc_ctx));
		static_assert(members.size() == 2, "Tagged union must have at least 2 members");

		if (!name) {
			std::string_view sv = stdm::display_string_of(type_info);
			if (const auto last_col = sv.rfind(':'); last_col != std::string_view::npos) {
				sv = sv.substr(last_col + 1);
			}
			name = sv.data();
		}

		using UnionTagTypeT = typename[:type_info:] ::xrTagType_t;
		template for (constexpr auto member_tag :
					  std::define_static_array(stdm::enumerators_of(stdm::type_of(members[0])))) {
			if (stdm::extract<UnionTagTypeT>(member_tag) == value.tag) {
				template for (constexpr stdm::info m : std::define_static_array(
								  stdm::nonstatic_data_members_of(stdm::type_of(members[1]), acc_ctx)
							  )) {
					constexpr auto annotations =
						std::define_static_array(stdm::annotations_of_with_type(m, stdm::dealias(^^UnionTagTypeT)));

					if (stdm::extract<UnionTagTypeT>(member_tag) == stdm::extract<UnionTagTypeT>(annotations[0])) {
						std::string_view tag_name = stdm::display_string_of(member_tag);
						if (const auto colon_pos = tag_name.rfind(':'); colon_pos != std::string_view::npos) {
							tag_name = tag_name.substr(colon_pos + 1);
						}

						XR_LOG_INFO(
							"TU member %s annotated with %s is active!", stdm::identifier_of(m).data(), tag_name.data()
						);

						config_setting_t* root_tu = config_setting_add(parent, name, CONFIG_TYPE_GROUP);
						serialize_object(root_tu, tag_name.data(), value.[:m:]);
						break;
					}
				}
			}
		}
	} else if constexpr (stdm::is_union_type(type_info)) {
		XR_LOG_INFO_FILE_LINE("Union %s", stdm::display_string_of(type_info).data());
	} else if constexpr (stdm::is_class_type(type_info)) {
		if (!name) {
			std::string_view sv = stdm::display_string_of(type_info);
			if (const auto last_col = sv.rfind(':'); last_col != std::string_view::npos) {
				sv = sv.substr(last_col + 1);
			}
			name = sv.data();
		}

		config_setting_t* obj_root = config_setting_add(parent, name, CONFIG_TYPE_GROUP);
		XRAY_ASSERT(obj_root != nullptr, "Parent must not be null");

		if constexpr (stdm::has_template_arguments(type_info)) {
			constexpr auto template_args = std::define_static_array(stdm::template_arguments_of(type_info));

			// XR_LOG_INFO("Serialize class template %s", stdm::display_string_of(type_info).data());
			// constexpr stdm::info type_templ = stdm::template_of(type_info);
			// XR_LOG_INFO("Template of %s", stdm::display_string_of(type_templ).data());

			if constexpr (template_args.size() > 1 && stdm::can_substitute(^^swl::variant, template_args) &&
						  stdm::substitute(^^swl::variant, template_args) == type_info) {
				template for (constexpr stdm::info arg :
							  std::define_static_array(stdm::template_arguments_of(type_info))) {
					constexpr stdm::info const_arg = stdm::add_const(arg);
					if (typename[:const_arg:]* val = swl::get_if<typename[:arg:]>(&value)) {
						// XR_LOG_INFO("Variant has type T: %s stored", stdm::display_string_of(arg).data());

						//
						// variant is serialized by writing the stringified stored type
						config_setting_t* var_stored_type	= config_setting_add(obj_root, "field", CONFIG_TYPE_STRING);
						constexpr std::string_view arg_name = stdm::display_string_of(arg);
						config_setting_set_string_with_len(var_stored_type, arg_name.data(), arg_name.size());
						//
						// and then the stored object
						serialize_object(obj_root, "value", *val);
						break;
					}
				}
			} else {
				//
				// non swl::variant type
				XR_LOG_INFO_FILE_LINE("MonkaS %s", stdm::display_string_of(type_info).data());
				// constexpr stdm::info templ_type = stdm::template_of(type_info);
				// constexpr auto annotations = stdm::annotations_of(templ_type);	//,
				// ^^xray::base::serialize_as_array); template for (constexpr auto a :
				// std::define_static_array(annotations)) { XR_LOG_INFO("Annotation %s",
				// stdm::display_string_of(a).data());
				// }

				template for (constexpr auto member :
							  std::define_static_array(stdm::nonstatic_data_members_of(type_info, acc_ctx))) {
					XR_LOG_INFO_FILE_LINE("[xx] %s", stdm::display_string_of(member).data());

					if constexpr (stdm::has_identifier(member)) {
						XR_LOG_INFO_FILE_LINE("Serialize %s", stdm::identifier_of(member).data());
						serialize_object(obj_root, stdm::identifier_of(member).data(), value.[:member:]);
					}
				}
			}
		} else {
			// XR_LOG_INFO_FILE_LINE("Non template type %s", stdm::display_string_of(type_info).data());
			//
			// non template type
			template for (constexpr auto member :
						  std::define_static_array(stdm::nonstatic_data_members_of(type_info, acc_ctx))) {
				serialize_object(obj_root, stdm::identifier_of(member).data(), value.[:member:]);
			}
		}
	} else if constexpr (stdm::is_array_type(type_info)) {
		serialize_array(parent, name, xray::base::slice_from_array(value));
	} else {
		serialize_one_element(parent, name, value);
	}
}

template <typename T>
bool deserialize_array(xray::base::MemoryArena& arena, const config_setting_t* parent, xray::base::xrSlice_t<T> arr);

template <typename T>
	requires AssociativeContainerLike<T>
bool deserialize_associative_container(xray::base::MemoryArena& arena, const config_setting_t* parent, T& cont);

template <typename T>
bool deserialize_object(xray::base::MemoryArena& arena, const config_setting_t* cfg_element, T& data) {
	constexpr stdm::info obj_type = stdm::dealias(stdm::remove_cvref(^^T));

	if constexpr (!stdm::annotations_of_with_type(obj_type, ^^xray::base::serialize_as_array).empty()) {
		template for ([[maybe_unused]] constexpr stdm::info annotation : std::define_static_array(
						  stdm::annotations_of_with_type(obj_type, ^^xray::base::serialize_as_array)
					  )) {
			XR_LOG_INFO_FILE_LINE(
				"%" PRI_xrStringView_t "annotated with %" PRI_xrStringView_t,
				FMT_xrStringView_t(stdm::display_string_of(obj_type)),
				FMT_xrStringView_t(stdm::display_string_of(annotation))
			);

			return deserialize_array(
				arena,
				cfg_element,
				xray::base::slice_from_ptr_and_len(data.data(), static_cast<xray::ISIZE>(data.size()))
			);
		}
	} else if constexpr (stdm::is_integral_type(obj_type)) {
		if constexpr (stdm::size_of(obj_type) < sizeof(xray::I32)) {
			data = static_cast<typename[:obj_type:]>(config_setting_get_bool(cfg_element));
		} else if constexpr (stdm::size_of(obj_type) < sizeof(xray::I64)) {
			data = static_cast<typename[:obj_type:]>(config_setting_get_int(cfg_element));
		} else {
			data = static_cast<typename[:obj_type:]>(config_setting_get_int64(cfg_element));
		}
	} else if constexpr (stdm::is_floating_point_type(obj_type)) {
		data = static_cast<typename[:obj_type:]>(config_setting_get_float(cfg_element));
	} else if constexpr (obj_type == stdm::dealias(^^std::string) ||
						 obj_type == stdm::dealias(^^std::filesystem::path)) {
		if (const char* s = config_setting_get_string(cfg_element)) {
			data.assign(s);
		}
	} else if constexpr (obj_type == ^^xray::base::xrString_t) {
		if (const char* s = config_setting_get_string(cfg_element)) {
			data = xray::base::string_from_c_str(arena, s);
		}
	} else if constexpr (stdm::is_array_type(obj_type)) {
		return deserialize_array(
			arena,
			cfg_element,
			xray::base::slice_from_ptr_and_len(&data[0], static_cast<xray::ISIZE>(stdm::extent(obj_type)))
		);
	} else if constexpr (SequenceContainerLike<typename[:obj_type:]>) {
		//
		// resize
		data.resize(static_cast<size_t>(config_setting_length(cfg_element)));
		return deserialize_array(
			arena, cfg_element, xray::base::slice_from_ptr_and_len(data.data(), static_cast<xray::ISIZE>(data.size()))
		);
	} else if constexpr (AssociativeContainerLike<typename[:obj_type:]>) {
		return deserialize_associative_container(arena, cfg_element, data);
	} else if constexpr (stdm::is_union_type(obj_type)) {
		XR_LOG_INFO("Union type");
	} else if constexpr (xrTaggedUnionLike<typename[:obj_type:]>) {
		constexpr stdm::access_context acc_ctx = stdm::access_context::current();

		constexpr auto members = std::define_static_array(stdm::nonstatic_data_members_of(obj_type, acc_ctx));
		static_assert(members.size() == 2, "Tagged union must have at least 2 members");

		XRAY_ASSERT_NOMSG(config_setting_type(cfg_element) == CONFIG_TYPE_GROUP);
		XRAY_ASSERT_NOMSG(config_setting_length(cfg_element) == 1);

		const config_setting_t* root = config_setting_get_elem(cfg_element, 0);
		const std::string_view active_field_name{config_setting_name(root)};
		XR_LOG_INFO(
			"[DESERIALIZE] Tagged union like: %s, active field %s",
			stdm::display_string_of(obj_type).data(),
			active_field_name.data()
		);

		using UnionTagTypeT = typename[:obj_type:] ::xrTagType_t;

		template for (constexpr stdm::info m :
					  std::define_static_array(stdm::nonstatic_data_members_of(stdm::type_of(members[1]), acc_ctx))) {
			constexpr auto annotations =
				std::define_static_array(stdm::annotations_of_with_type(m, stdm::dealias(^^UnionTagTypeT)));
			if (constexpr std::string_view tag_name = stdm::display_string_of(stdm::constant_of(annotations[0]));
				tag_name.find(active_field_name) != std::string_view::npos) {
				XR_LOG_INFO_FILE_LINE("[DESERIALIZE] serialized field %s", tag_name.data());

				typename[:type_of(m):] stored_val;
				if (!deserialize_object(arena, root, stored_val)) {
					return false;
				}

				data.tag   = stdm::extract<UnionTagTypeT>(stdm::constant_of(annotations[0]));
				data.[:m:] = std::move(stored_val);
				return true;
				// XR_LOG_INFO_FILE_LINE("Union tag = %d", std::to_underlying(data.tag));
			}
		}
	} else if constexpr (stdm::is_class_type(obj_type)) {
		const auto setting_ty = config_setting_type(cfg_element);
		XRAY_ASSERT(
			setting_ty == CONFIG_TYPE_GROUP,
			"Expected setting type group for object %s",
			stdm::display_string_of(obj_type).data()
		);

		if (setting_ty != CONFIG_TYPE_GROUP) {
			return false;
		}

		if constexpr (stdm::has_template_arguments(obj_type)) {
			constexpr auto template_args = std::define_static_array(stdm::template_arguments_of(obj_type));

			// XR_LOG_INFO("Serialize class template %s", stdm::display_string_of(type_info).data());
			// constexpr stdm::info type_templ = stdm::template_of(type_info);
			// XR_LOG_INFO("Template of %s", stdm::display_string_of(type_templ).data());

			if constexpr (template_args.size() > 1 && stdm::can_substitute(^^swl::variant, template_args) &&
						  stdm::substitute(^^swl::variant, template_args) == obj_type) {
				template for (constexpr stdm::info arg :
							  std::define_static_array(stdm::template_arguments_of(obj_type))) {
					// XR_LOG_INFO("Variant has type T: %s stored", stdm::display_string_of(arg).data());

					//
					// variant is serialized by writing the stringified stored type
					const config_setting_t* var_stored_type = config_setting_get_elem(cfg_element, 0);
					const config_setting_t* var_stored_val	= config_setting_get_elem(cfg_element, 1);

					const std::string_view stored_type_desc = config_setting_get_string(var_stored_type);
					if (stored_type_desc == stdm::display_string_of(arg)) {
						typename[:arg:] stored_value{};
						if (!deserialize_object(arena, var_stored_val, stored_value)) {
							return false;
						}

						data = stored_value;
						break;
					}
				}
			} else {
				XR_LOG_INFO_FILE_LINE("Serialize non SWL template type %s", stdm::display_string_of(obj_type).data());
				//
				// non swl::variant type
				constexpr stdm::access_context acc_ctx = stdm::access_context::current();
				template for (constexpr stdm::info data_member :
							  std::define_static_array(stdm::nonstatic_data_members_of(obj_type, acc_ctx))) {
					if constexpr (stdm::has_identifier(data_member)) {
						const config_setting_t* group_member =
							config_setting_get_member(cfg_element, stdm::identifier_of(data_member).data());
						if (!group_member) {
							continue;
						}

						if (!deserialize_object(arena, group_member, data.[:data_member:])) {
							return false;
						}
					}
				}
			}
		} else {
			XR_LOG_INFO_FILE_LINE("Non template type %s", stdm::display_string_of(obj_type).data());
			//
			// non template type
			constexpr stdm::access_context acc_ctx = stdm::access_context::current();
			template for (constexpr stdm::info data_member :
						  std::define_static_array(stdm::nonstatic_data_members_of(obj_type, acc_ctx))) {
				XR_LOG_INFO_FILE_LINE("member: %s", stdm::display_string_of(data_member).data());
				if constexpr (stdm::has_identifier(data_member)) {
					const config_setting_t* group_member =
						config_setting_get_member(cfg_element, stdm::identifier_of(data_member).data());
					if (!group_member) {
						continue;
					}

					if (!deserialize_object(arena, group_member, data.[:data_member:])) {
						return false;
					}
				}
			}
		}
	} else {
		static_assert(xray::minstd::dependant_always_false_v<typename[:obj_type:]>, "Unhandled type!");
		return false;
	}

	return true;
}

template <typename T>
bool deserialize_array(xray::base::MemoryArena& arena, const config_setting_t* parent, xray::base::xrSlice_t<T> arr) {
	using namespace xray;
	const ISIZE cfg_arr_len = config_setting_length(parent);
	for (ISIZE idx = 0; idx < minstd::min_of(cfg_arr_len, arr.s_len); ++idx) {
		const config_setting_t* e = config_setting_get_elem(parent, static_cast<unsigned int>(idx));
		if (!deserialize_object(arena, e, arr[idx])) {
			return false;
		}
	}

	return true;
}

template <typename T>
	requires AssociativeContainerLike<T>
bool deserialize_associative_container(xray::base::MemoryArena& arena, const config_setting_t* parent, T& cont) {
	XRAY_ASSERT(
		(config_setting_type(parent) == CONFIG_TYPE_LIST),
		"Associative containers need to be of list type, got %d",
		config_setting_type(parent)
	);

	const xray::ISIZE element_count = config_setting_length(parent);
	for (xray::ISIZE idx = 0; idx < element_count; ++idx) {
		const config_setting_t* entry = config_setting_get_elem(parent, static_cast<unsigned int>(idx));
		if (!entry) {
			return false;
		}

		XRAY_ASSERT(
			(config_setting_type(entry)),
			"Associative container element needs to be of type group, but has type %d",
			config_setting_type(entry)
		);

		const config_setting_t* key_entry = config_setting_get_elem(entry, 0);
		const config_setting_t* val_entry = config_setting_get_elem(entry, 1);

		XRAY_ASSERT((key_entry != nullptr && val_entry != nullptr), "Need to have 2 entries (1 key, 2 value)");

		using key_type	  = typename T::key_type;
		using mapped_type = typename T::mapped_type;

		key_type key{};
		if (!deserialize_object(arena, key_entry, key)) {
			return false;
		}

		mapped_type val{};
		if (!deserialize_object(arena, val_entry, val)) {
			return false;
		}

		cont[key] = XRAY_MOVE(val);
	}

	return true;
}

}  // namespace

template <typename T>
xray::base::xrSerializeResult_t xray::base::serialize_to_file(const T& value, const xray::base::xrStringView_t file) {
	config_t cfg{};
	config_init(&cfg);
	XRAY_SCOPE_EXIT_NOEXCEPT { config_destroy(&cfg); };

	config_setting_t* root = config_root_setting(&cfg);
	serialize_object(root, nullptr, value);
	const auto write_result = config_write_file(&cfg, file.cdata());
	if (write_result != CONFIG_TRUE) {
		//
		// dump error
		return xrSerializeResult_t::Error;
	}
	return xrSerializeResult_t::Success;
}

template <typename T>
bool xray::base::deserialize(xray::base::MemoryArena& arena, const xray::C8* str, T& data) {
	config_t cfg{};
	XRAY_SCOPE_EXIT_NOEXCEPT { config_destroy(&cfg); };

	if (config_read_string(&cfg, str) != CONFIG_TRUE) {
		XR_LOG_ERR(
			"Deserialization error:%d: %s:%d", config_error_type(&cfg), config_error_text(&cfg), config_error_line(&cfg)
		);
		return false;
	}

	config_setting_t* root = config_root_setting(&cfg);
	if (config_setting_type(root) != CONFIG_TYPE_GROUP) {
		return false;
	}

	root = config_setting_get_elem(root, 0);
	return deserialize_object(arena, root, data);
}

template <typename T>
bool xray::base::deserialize_from_file(xray::base::MemoryArena& arena, T& data, const xray::base::xrStringView_t file) {
	FILE* f = fopen(file.cdata(), "rt");
	if (!f) {
		XR_LOG_ERR_FILE_LINE("Failed to open file %" PRI_xrStringView_t, FMT_xrStringView_t(file));
		return false;
	}

	XRAY_SCOPE_EXIT_NOEXCEPT { fclose(f); };
	using namespace xray;
	fseek(f, 0, SEEK_END);
	const ISIZE file_size = ftell(f);
	fseek(f, 0, SEEK_SET);

	base::ScratchPadArena spad		  = base::ThreadLocalContext::acquire_scratchpad({&arena});
	base::xrSlice_t<C8> file_contents = spad.arena->alloc_align<C8>(file_size + 1);

	const size_t bytes_read =
		fread_unlocked(file_contents.s_ptr, sizeof(C8), static_cast<size_t>(file_contents.s_len), f);
	if (bytes_read == 0) {
		return false;
	}

	file_contents[bytes_read] = 0;
	return deserialize(arena, file_contents.s_ptr, data);
}

template <typename T>
bool xray::base::serialize_to_memory(const T& data, xray::base::xrSlice_t<xray::U8> mem) {
	xrFileMemStream_t fs = xrFileMemStream_t::create(mem);
	if (!fs) {
		return false;
	}

	XRAY_SCOPE_EXIT_NOEXCEPT { fs.destroy(); };

	config_t cfg{};
	config_init(&cfg);
	XRAY_SCOPE_EXIT_NOEXCEPT { config_destroy(&cfg); };

	serialize_object(config_root_setting(&cfg), nullptr, data);
	config_write(&cfg, fs.fp_);
	return true;
}
