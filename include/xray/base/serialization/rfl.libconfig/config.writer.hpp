#pragma once

#include <string>
#include <cstdint>
#include <string_view>
#include <type_traits>

#include <libconfig.h>
#include <rfl/always_false.hpp>

namespace rfl ::libconfig {

class Writer
{
  public:
    struct LibconfigOutputArray
    {
        LibconfigOutputArray(config_setting_t* _val)
            : val_(_val)
        {
        }
        config_setting_t* val_;
    };

    struct LibconfigOutputObject
    {
        LibconfigOutputObject(config_setting_t* _val)
            : val_(_val)
        {
        }
        config_setting_t* val_;
    };

    struct LibconfigOutputVar
    {
        LibconfigOutputVar(config_setting_t* _val)
            : val_(_val)
        {
        }

        LibconfigOutputVar(LibconfigOutputArray _arr)
            : val_(_arr.val_)
        {
        }

        LibconfigOutputVar(LibconfigOutputObject _obj)
            : val_(_obj.val_)
        {
        }

        config_setting_t* val_;
    };

    using OutputArrayType = LibconfigOutputArray;
    using OutputObjectType = LibconfigOutputObject;
    using OutputVarType = LibconfigOutputVar;

    Writer(config_t* _doc);

    OutputArrayType array_as_root(const size_t) const noexcept;
    OutputObjectType object_as_root(const size_t) const noexcept;
    OutputVarType null_as_root() const noexcept;

    template<class T>
    OutputVarType value_as_root(const T& _var) const noexcept
    {
        config_setting_t* root = config_root_setting(doc_);
        const auto val = from_basic_type(_var);
        using ValueType = std::remove_cvref_t<T>;
        if constexpr (std::is_same_v<ValueType, std::string>) {
            config_setting_set_string(root, _var.c_str());
        } else if (std::is_same_v<ValueType, bool>) {
            config_setting_set_bool(root, _var);
        } else if (std::is_floating_point_v<ValueType>) {
            config_setting_set_float(root, _var);
        } else if (std::is_integral_v<ValueType>) {
            if constexpr (sizeof(ValueType) > 4) {
                config_setting_set_int64(root, _var);
            } else {
                config_setting_set_int(root, _var);
            }
        } else {
            static_assert(rfl::always_false_v<ValueType>, "Unsupported type.");
        }
        return OutputVarType{ root };
    }

    OutputArrayType add_array_to_array(const size_t, OutputArrayType* _parent) const noexcept;
    OutputArrayType add_array_to_object(const std::string_view& _name,
                                        const size_t,
                                        OutputObjectType* _parent) const noexcept;
    OutputObjectType add_object_to_array(const size_t, OutputArrayType* _parent) const noexcept;
    OutputObjectType add_object_to_object(const std::string_view& _name,
                                          const size_t,
                                          OutputObjectType* _parent) const noexcept;

    template<class T>
    OutputVarType add_value_to_array(const T& _var, OutputArrayType* _parent) const noexcept
    {
        using ValueType = std::remove_cvref_t<T>;
        if constexpr (std::is_same_v<ValueType, std::string>) {
            config_setting_set_string_elem(_parent->val_, -1, _var.c_str());
        } else if constexpr (std::is_same_v<ValueType, bool>) {
            config_setting_set_bool_elem(_parent->val_, -1, _var);
        } else if constexpr (std::is_floating_point_v<ValueType>) {
            config_setting_set_float_elem(_parent->val_, -1, _var);
        } else if constexpr (std::is_integral_v<ValueType>) {
            if constexpr (sizeof(ValueType) > 4) {
                config_setting_set_int64_elem(_parent->val_, -1, _var);
            } else {
                config_setting_set_int_elem(_parent->val_, -1, _var);
            }
        } else {
            static_assert(rfl::always_false_v<ValueType>, "Unsupported type.");
        }

        const int32_t idx = config_setting_length(_parent->val_);
        return OutputVarType{ config_setting_get_elem(_parent->val_, idx - 1) };
    }

    template<class T>
    OutputVarType add_value_to_object(const std::string_view& _name,
                                      const T& _var,
                                      OutputObjectType* _parent) const noexcept
    {
        using ValueType = std::remove_cvref_t<T>;
        config_setting_t* s{};
        if constexpr (std::is_same_v<ValueType, std::string>) {
            s = config_setting_add(_parent->val_, _name.data(), CONFIG_TYPE_STRING);
            config_setting_set_string(s, _var.c_str());
        } else if constexpr (std::is_same_v<ValueType, bool>) {
            s = config_setting_add(_parent->val_, _name.data(), CONFIG_TYPE_BOOL);
            config_setting_set_bool(s, _var);
        } else if constexpr (std::is_floating_point_v<ValueType>) {
            s = config_setting_add(_parent->val_, _name.data(), CONFIG_TYPE_FLOAT);
            config_setting_set_float(s, _var);
        } else if constexpr (std::is_integral_v<ValueType>) {
            if constexpr (sizeof(ValueType) > 4) {
                s = config_setting_add(_parent->val_, _name.data(), CONFIG_TYPE_INT64);
                config_setting_set_int64(s, _var);
            } else {
                s = config_setting_add(_parent->val_, _name.data(), CONFIG_TYPE_INT);
                config_setting_set_int(s, _var);
            }
        } else {
            static_assert(rfl::always_false_v<ValueType>, "Unsupported type.");
        }

        return OutputVarType{ s };
    }

    OutputVarType add_null_to_array(OutputArrayType* _parent) const noexcept { return OutputVarType{ nullptr }; }
    OutputVarType add_null_to_object(const std::string_view& _name, OutputObjectType* _parent) const noexcept
    {
        return OutputVarType{ nullptr };
    }

    void end_array(OutputArrayType*) const noexcept {}
    void end_object(OutputObjectType*) const noexcept {}

  public:
    config_t* doc_;
    mutable uint32_t count_objs_{};
    mutable uint32_t count_array_{};
};
} // namespace rfl::libconfig
