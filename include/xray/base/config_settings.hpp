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

/// \file   config_settings.hpp  Classes for reading and writing structured
///                     configuration files.

#include "xray/base/array_dimension.hpp"
#include "xray/base/maybe.hpp"
#include "xray/xray.hpp"
//#include "xray/base/shims/stl_type_traits_shims.hpp"
//#include <algorithm>
#include <cassert>
#include <cstdint>
#include <libconfig.h>
#include <string>

namespace xray {
namespace base {

/// \addtogroup __GroupXrayBase
/// @{

/// \enum config_entry_type
/// Types supported by the configuration files.
enum class config_entry_type
{
    none,
    group,
    integer,
    integer64,
    array,
    string,
    floating_point,
    boolean,
    list
};

/// @}

namespace detail {

template<typename T>
struct config_val_helper;

template<>
struct config_val_helper<const char*>
{
    static const config_entry_type type;

    static const char* get(config_setting_t* s) noexcept { return config_setting_get_string(s); }

    static const char* get(config_setting_t* s, const int32_t idx) noexcept
    {
        return config_setting_get_string_elem(s, idx);
    }
};

inline config_entry_type
libconfig_type_to_entry_type(config_setting_t* s) noexcept
{
    switch (config_setting_type(s)) {
        case CONFIG_TYPE_INT:
            return config_entry_type::integer;
            break;

        case CONFIG_TYPE_ARRAY:
            return config_entry_type::array;
            break;

        case CONFIG_TYPE_GROUP:
            return config_entry_type::group;
            break;

        case CONFIG_TYPE_INT64:
            return config_entry_type::integer64;
            break;

        case CONFIG_TYPE_STRING:
            return config_entry_type::string;
            break;

        case CONFIG_TYPE_FLOAT:
            return config_entry_type::floating_point;
            break;

        case CONFIG_TYPE_BOOL:
            return config_entry_type::boolean;
            break;

        case CONFIG_TYPE_LIST:
            return config_entry_type::list;

        default:
            break;
    }

    return config_entry_type::none;
}

inline void
set_value(config_setting_t* s, const float val) noexcept
{
    config_setting_set_float(s, val);
}

inline void
set_value(config_setting_t* s, const int32_t val) noexcept
{
    config_setting_set_int(s, val);
}

inline void
set_value(config_setting_t* s, const char* c_str) noexcept
{
    config_setting_set_string(s, c_str);
}

inline void
set_value(config_setting_t* s, const int64_t val) noexcept
{
    config_setting_set_int64(s, val);
}

inline void
get_value(const config_setting_t* s, int32_t& val) noexcept
{
    val = config_setting_get_int(s);
}

inline void
get_value(const config_setting_t* s, int64_t& val) noexcept
{
    val = config_setting_get_int64(s);
}

inline void
get_value(const config_setting_t* s, float& val) noexcept
{
    val = static_cast<float>(config_setting_get_float(s));
}

inline void
get_value(const config_setting_t* s, const char*& val) noexcept
{
    val = config_setting_get_string(s);
}

inline bool
lookup_value(config_t* cfg, const char* path, bool& value) noexcept
{
    int32_t tmp_bool{};
    if (config_lookup_bool(cfg, path, &tmp_bool) != CONFIG_TRUE)
        return false;

    value = (tmp_bool == 0 ? false : true);
    return true;
}

inline bool
lookup_value(config_t* cfg, const char* path, int32_t& value) noexcept
{
    int32_t tmp_val;
    if (config_lookup_int(cfg, path, &tmp_val) != CONFIG_TRUE)
        return false;

    value = static_cast<int32_t>(tmp_val);
    return true;
}

inline bool
lookup_value(config_t* cfg, const char* path, uint32_t& value) noexcept
{
    long long tmp_val;
    if (config_lookup_int64(cfg, path, &tmp_val) != CONFIG_TRUE || tmp_val < 0)
        return false;

    assert(tmp_val < static_cast<uint32_t>(-1));

    value = static_cast<uint32_t>(tmp_val);
    return true;
}

inline bool
lookup_value(config_t* cfg, const char* path, int64_t& value) noexcept
{
    long long tmp_val;
    if (config_lookup_int64(cfg, path, &tmp_val) != CONFIG_TRUE)
        return false;

    value = tmp_val;
    return true;
}

inline bool
lookup_value(config_t* cfg, const char* path, const char*& str) noexcept
{
    return config_lookup_string(cfg, path, &str) == CONFIG_TRUE;
}

inline bool
lookup_value(config_t* cfg, const char* path, std::string& str) noexcept
{
    const char* sval;
    if (!lookup_value(cfg, path, sval))
        return false;

    str.assign(sval);
    return true;
}

inline bool
lookup_value(config_t* cfg, const char* path, float& value) noexcept
{
    double dbl_val;
    if (config_lookup_float(cfg, path, &dbl_val) != CONFIG_TRUE)
        return false;

    value = static_cast<float>(dbl_val);
    return true;
}

int32_t
setting_type_to_library_type(const config_entry_type st) noexcept;

config_entry_type
library_type_to_setting_type(const int32_t lt) noexcept;

template<typename T>
struct type_to_setting_type
{
    static const config_entry_type tp = config_entry_type::none;
    static const int32_t lty = CONFIG_TYPE_NONE;
};

template<>
struct type_to_setting_type<int32_t>
{
    static const config_entry_type tp = config_entry_type::integer;
    static const int32_t lty = CONFIG_TYPE_INT;
};

template<>
struct type_to_setting_type<int64_t>
{
    static const config_entry_type tp = config_entry_type::integer64;
    static const int32_t lty = CONFIG_TYPE_INT64;
};

template<typename T>
struct type_to_setting_type<T[]>
{
    static const config_entry_type tp = config_entry_type::array;
    static const int32_t lty = CONFIG_TYPE_ARRAY;
};

template<>
struct type_to_setting_type<const char*>
{
    static const config_entry_type tp = config_entry_type::string;
    static const int32_t lty = CONFIG_TYPE_STRING;
};

template<>
struct type_to_setting_type<float>
{
    static const config_entry_type tp = config_entry_type::floating_point;
    static const int32_t lty = CONFIG_TYPE_FLOAT;
};

} // namespace detail

/// \addtogroup __GroupXrayBase
/// @{

class config_file;

///
/// \brief An entry in the configuration file. It can be a group, a single value
/// (integer, floating point, string) or an aggregate (array of integers,
/// strings, etc).
///
/// \code
/// const char *output_file = "simple_config.conf";
/// config_file cfg;
/// config_file_entry conf_root = cfg.root();
///
/// config_file_entry cam_grp = conf_root.add_child("camera_setup",
/// config_entry_type::group);
/// config_file_entry cam_pos = cam_grp.add_child("origin",
/// config_entry_type::array);
///
/// float3 p{ 0.0f, 0.0f, -10.0f };
/// cam_pos.set_vector_or_matrix(p);
///
/// const char* filename = "assets/f4-phantom.3ds";
/// config_file_entry obj_file = conf_root.add_child("model_file",
/// config_entry_type::string);
/// obj_file.set_value(filename);
///
/// cfg.write_file(output_file);
/// \endcode
class config_file_entry
{
  private:
    config_file_entry(config_setting_t* s, const config_entry_type type) noexcept
        : setting_{ s }
        , type_{ type }
    {
    }

    friend class config_file;

  public:
    inline config_file_entry add_child(const char* name, const config_entry_type type) noexcept;

    template<typename T>
    void set_value(const T val) noexcept
    {
        assert(valid());
        assert(type_ == xray::base::detail::type_to_setting_type<T>::tp);

        xray::base::detail::set_value(setting_, val);
    }

    template<typename T>
    void set_value(const T* arr_elements, const size_t num_elements) noexcept
    {
        assert(valid());
        assert(type_ == config_entry_type::array);

        for (size_t idx = 0; idx < num_elements; ++idx) {
            config_setting_t* arr_elem = config_setting_get_elem(setting_, static_cast<unsigned int>(idx));

            if (!arr_elem) {
                arr_elem = config_setting_add(setting_, nullptr, xray::base::detail::type_to_setting_type<T>::lty);
            }
            xray::base::detail::set_value(arr_elem, arr_elements[idx]);
        }
    }

    template<typename T, size_t N>
    void set_value(const T (&arr_ref)[N]) noexcept
    {
        set_value(&arr_ref[0], N);
    }

    template<typename vector_type>
    void set_vector_or_matrix(const vector_type& vec) noexcept
    {
        set_value(vec.components, XR_COUNTOF(vec.components));
    }

    bool valid() const noexcept { return setting_ != nullptr && type_ != config_entry_type::none; }

    template<typename T>
    void get_value(T& output_val) const noexcept
    {
        assert(valid());
        assert(type_ == xray::base::detail::type_to_setting_type<T>::tp);

        xray::base::detail::get_value(setting_, output_val);
    }

    template<typename T>
    void get_value(T* output_arr, const size_t max_elements) const noexcept
    {
        assert(valid());
        assert(type_ == config_entry_type::array);

        const int32_t arr_len = config_setting_length(setting_);
        if (arr_len < 1)
            return;

        const uint32_t max_count = std::max(static_cast<uint32_t>(arr_len), static_cast<uint32_t>(max_elements));

        for (uint32_t idx = 0; idx < max_count; ++idx) {
            config_setting_t* arr_elem = config_setting_get_elem(setting_, idx);
            xray::base::detail::get_value(arr_elem, output_arr[idx]);
        }
    }

    template<typename T, size_t N>
    void get_value(T (&arr_ref)[N]) const noexcept
    {
        get_value(&arr_ref[0], N);
    }

    template<typename T>
    void get_vector_or_matrix(T& vec_mtx) const noexcept
    {
        get_value(vec_mtx.Elements);
    }

  private:
    config_setting_t* setting_;
    config_entry_type type_;
};

inline config_file_entry
config_file_entry::add_child(const char* name, const config_entry_type type) noexcept
{
    config_setting_t* child =
        config_setting_add(setting_, name, xray::base::detail::setting_type_to_library_type(type));

    return config_file_entry{ child, type };
}

class config_entry
{
  private:
    friend class config_file;

    explicit config_entry(config_setting_t* s) noexcept
        : setting_{ s }
        , type_{ s ? base::detail::libconfig_type_to_entry_type(s) : config_entry_type::none }
    {
    }

  public:
    bool is_array() const noexcept
    {
        assert(valid());
        return type_ == config_entry_type::array;
    }

    bool valid() const noexcept { return setting_ != nullptr && type_ != config_entry_type::none; }

    explicit operator bool() const noexcept { return valid(); }

  public:
    config_entry operator[](const uint32_t idx) const noexcept
    {
        assert(valid());
        assert(type_ == config_entry_type::array || type_ == config_entry_type::list);

        return config_entry{ config_setting_get_elem(setting_, idx) };
    }

    float as_float() const noexcept
    {
        assert(valid());
        assert(type_ == config_entry_type::floating_point);

        return static_cast<float>(config_setting_get_float(setting_));
    }

    int32_t as_int32() const noexcept
    {
        assert(valid());
        assert(type_ == config_entry_type::integer);

        return static_cast<int32_t>(config_setting_get_int(setting_));
    }

    int64_t as_int64() const noexcept
    {
        assert(valid());
        assert(type_ == config_entry_type::integer64);

        return static_cast<int64_t>(config_setting_get_int64(setting_));
    }

    bool as_bool() const noexcept
    {
        assert(valid());
        assert(type_ == config_entry_type::boolean);
        return config_setting_get_bool(setting_) != 0;
    }

    const char* as_string() const noexcept
    {
        assert(valid());
        assert(type_ == config_entry_type::string);

        return config_setting_get_string(setting_);
    }

    uint32_t length() const noexcept
    {
        assert(valid());
        assert(type_ == config_entry_type::array || type_ == config_entry_type::list ||
               type_ == config_entry_type::group);

        return static_cast<uint32_t>(config_setting_length(setting_));
    }

    float float_at(const uint32_t idx) const noexcept
    {
        assert(valid());
        assert(type_ == config_entry_type::array);

        return static_cast<float>(config_setting_get_float_elem(setting_, static_cast<int32_t>(idx)));
    }

    int32_t int_at(const uint32_t idx) const noexcept
    {
        assert(valid());
        assert(type_ == config_entry_type::array);

        return config_setting_get_int_elem(setting_, static_cast<int32_t>(idx));
    }

    int64_t int64_at(const uint32_t idx) const noexcept
    {
        assert(valid());
        assert(type_ == config_entry_type::array);

        return config_setting_get_int64_elem(setting_, static_cast<int32_t>(idx));
    }

    const char* string_at(const uint32_t idx) const noexcept
    {
        assert(valid());
        assert(type_ == config_entry_type::array);

        return config_setting_get_string_elem(setting_, static_cast<int32_t>(idx));
    }

    bool bool_at(const uint32_t idx) const noexcept
    {
        assert(valid());
        assert(type_ == config_entry_type::array);

        return config_setting_get_bool_elem(setting_, static_cast<int32_t>(idx)) != 0;
    }

    config_entry lookup(const char* path) const noexcept
    {
        assert(valid());
        assert(path != nullptr);
        return config_entry{ config_setting_lookup(setting_, path) };
    }

    maybe<float> lookup_float(const char* path) const noexcept
    {
        assert(valid());
        assert(path != nullptr);

        double val;
        if (config_setting_lookup_float(setting_, path, &val) != CONFIG_TRUE) {
            return nothing{};
        }

        return static_cast<float>(val);
    }

    maybe<int32_t> lookup_int32(const char* path) const noexcept
    {
        assert(valid());
        assert(path != nullptr);

        int32_t val;
        if (config_setting_lookup_int(setting_, path, &val) != CONFIG_TRUE)
            return nothing{};

        return val;
    }

    maybe<uint32_t> lookup_uint32(const char* path) const noexcept
    {
        assert(valid());
        assert(path != nullptr);

        int32_t val{};
        if (config_setting_lookup_int(setting_, path, &val) != CONFIG_TRUE)
            return nothing{};

        return static_cast<uint32_t>(val);
    }

    maybe<bool> lookup_bool(const char* path) const noexcept
    {
        assert(valid());
        assert(path != nullptr);

        int32_t bval{};
        if (config_setting_lookup_bool(setting_, path, &bval) != CONFIG_TRUE)
            return nothing{};

        return bval == 1;
    }

    maybe<int64_t> lookup_int64(const char* path) const noexcept
    {
        assert(valid());
        assert(path != nullptr);

        long long int val{};
        if (config_setting_lookup_int64(setting_, path, &val) != CONFIG_TRUE)
            return nothing{};

        return { static_cast<int64_t>(val) };
    }

    maybe<uint64_t> lookup_uint64(const char* path) const noexcept
    {
        assert(valid());
        assert(path != nullptr);

        long long int val{};
        if (config_setting_lookup_int64(setting_, path, &val) != CONFIG_TRUE)
            return nothing{};

        return { static_cast<uint64_t>(val) };
    }

    maybe<const char*> lookup_string(const char* path) const noexcept
    {
        assert(valid());
        assert(path != nullptr);

        const char* val{};
        if (config_setting_lookup_string(setting_, path, &val) != CONFIG_TRUE)
            return nothing{};

        return val;
    }

  private:
    config_setting_t* setting_;
    config_entry_type type_;
};

///
/// \brief  Abstract representation of a structured configuration file.
/// \code
/// config_file camera_params;
/// if (!camera_params.read_file("cam_params.conf"))
///     return false;
///
/// float3 cam_x_axis;
/// camera_params.lookup_vector_or_matrix("cam_params.x_axis", cam_x_axis);
///
/// float cam_max_spd;
/// camera_params.lookup_value("cam_params.max_speed", cam_max_spd);
///
/// matrix_3X3F cam_orientation;
/// camera_params.lookup_vector_or_matrix("cam_params.orientation",
/// cam_orientation);
/// ...
/// \endcode
class config_file
{
  public:
    config_file() { config_init(&conf_); }

    explicit config_file(const char* file_name)
        : config_file{}
    {
        assert(file_name != nullptr);
        read_file(file_name);
    }

    ~config_file() { config_destroy(&conf_); }

    explicit operator bool() const noexcept { return valid_; }
    bool valid() const noexcept { return valid_; }

    config_entry root() const noexcept { return config_entry{ config_root_setting(&conf_) }; }

    config_entry lookup_entry(const char* path) const noexcept { return config_entry{ config_lookup(&conf_, path) }; }

    template<typename T>
    bool lookup_value(const char* path, T& output_val) noexcept;

    template<typename T, size_t N>
    bool lookup_value(const char* path, T (&arr_ref)[N]) noexcept
    {
        return lookup_value(path, &arr_ref[0], N);
    }

    template<typename T>
    bool lookup_value(const char* path, T* arr_output, const size_t max_elements) noexcept;

    template<typename T>
    bool lookup_vector_or_matrix(const char* path, T& vec_mtx) noexcept
    {
        return lookup_value(path, vec_mtx.components);
    }

    inline bool write_file(const char* file_name) noexcept;

    inline bool read_file(const char* file_name) noexcept;

    std::string error() const noexcept;

  public:
    config_t conf_;
    bool valid_{ false };

  private:
    XRAY_NO_COPY(config_file);
};

template<typename T>
bool
config_file::lookup_value(const char* path, T& output_val) noexcept
{
    return ::xray::base::detail::lookup_value(&conf_, path, output_val);
}

template<typename T>
bool
config_file::lookup_value(const char* path, T* arr_output, const size_t max_elements) noexcept
{
    config_setting_t* arr_entry = config_lookup(&conf_, path);
    if (!arr_entry || !config_setting_is_array(arr_entry))
        return false;

    const int32_t arr_len = config_setting_length(arr_entry);
    if (arr_len < 1)
        return false;

    const size_t elem_cnt = std::min(max_elements, static_cast<size_t>(arr_len));

    for (size_t idx = 0; idx < elem_cnt; ++idx) {
        config_setting_t* arr_element = config_setting_get_elem(arr_entry, static_cast<uint32_t>(idx));
        xray::base::detail::get_value(arr_element, arr_output[idx]);
    }

    return true;
}

inline bool
config_file::write_file(const char* file_name) noexcept
{
    return config_write_file(&conf_, file_name) == CONFIG_TRUE;
}

inline bool
config_file::read_file(const char* file_name) noexcept
{
    valid_ = config_read_file(&conf_, file_name) == CONFIG_TRUE;
    return valid_;
}

/// @}

} // namespace base
} // namespace xray
