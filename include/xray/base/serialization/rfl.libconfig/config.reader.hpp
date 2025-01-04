#pragma once

#include <libconfig.h>
#include <rfl/Result.hpp>
#include <rfl/always_false.hpp>

namespace rfl::libconfig {
struct Reader
{
    using InputArrayType = config_setting_t*;
    using InputObjectType = config_setting_t*;
    using InputVarType = config_setting_t*;

    /// If you do not want to support custom constructors,
    /// just set this to false.
    template<class T>
    static constexpr bool has_custom_constructor = false;

    /// Retrieves a particular field from an array.
    /// Returns an rfl::Error if the index is out of bounds.
    /// If your format is schemaful, you do not need this.
    rfl::Result<InputVarType> get_field_from_array(const size_t _idx, const InputArrayType _arr) const noexcept
    {
        config_setting_t* s = config_setting_get_elem(_arr, _idx);
        if (s)
            return s;
        else
            return rfl::Error("cant find field @idx");
    }

    /// Retrieves a particular field from an object.
    /// Returns an rfl::Error if the field cannot be found.
    /// If your format is schemaful, you do not need this.
    rfl::Result<InputVarType> get_field_from_object(const std::string& _name,
                                                    const InputObjectType& _obj) const noexcept
    {
        config_setting_t* s = config_setting_get_member(_obj, _name.c_str());
        if (s)
            return s;
        else
            return rfl::Error("cant find field");
    }

    /// Determines whether a variable is empty (the NULL type).
    bool is_empty(const InputVarType& _var) const noexcept { return config_setting_length(_var) == 0; }

    /// Cast _var as a basic type (bool, integral,
    /// floating point, std::string).
    /// Returns an rfl::Error if it cannot be cast
    /// as that type
    template<class T>
    rfl::Result<T> to_basic_type(const InputVarType& _var) const noexcept
    {
        using ValType = std::remove_cvref_t<T>;
        if constexpr (std::is_same_v<T, std::string>) {
            const char* s = config_setting_get_string(_var);
            return s ? rfl::Result<T>{ std::string{ s } } : rfl::Error("not a string");
        } else if constexpr (std::is_same_v<T, bool>) {
            return config_setting_get_bool(_var) != 0;
        } else if constexpr (std::is_floating_point_v<T>) {
            return static_cast<T>(config_setting_get_float(_var));
        } else if constexpr (std::is_integral_v<T>) {
            return static_cast<T>(config_setting_get_int64(_var));
        } else {
            static_assert(rfl::always_false_v<ValType>, "Not supported");
        }
    }

    /// Casts _var as an InputArrayType.
    /// Returns an rfl::Error if `_var` cannot be cast as an array.
    rfl::Result<InputArrayType> to_array(const InputVarType& _var) const noexcept
    {
        if (config_setting_is_list(_var))
            return _var;
        else
            return rfl::Error("not an array");
    }

    /// Casts _var as an InputObjectType.
    /// Returns an rfl::Error if `_var` cannot be cast as an object.
    rfl::Result<InputObjectType> to_object(const InputVarType& _var) const noexcept
    {
        if (config_setting_is_group(_var))
            return _var;
        return rfl::Error("not an object");
    }

    /// Iterates through an array and inserts the values into the array
    /// reader. See below for a more detailed explanation.
    template<class ArrayReader>
    std::optional<Error> read_array(const ArrayReader& _array_reader, const InputArrayType& _arr) const noexcept
    {
        const int32_t count = config_setting_length(_arr);
        for (int32_t i = 0; i < count; ++i) {
            config_setting_t* e = config_setting_get_elem(_arr, i);
            if (!e)
                return rfl::Error("error reading array");

            const auto err = _array_reader.read(e);
            if (err)
                return err;
        }

        return std::nullopt;
    }

    /// Iterates through an object and inserts the key-value pairs into the object
    /// reader. See below for a more detailed explanation.
    template<class ObjectReader>
    std::optional<Error> read_object(const ObjectReader& _object_reader, const InputObjectType& _obj) const noexcept
    {
        if (!config_setting_is_group(_obj))
            return rfl::Error("Not an object");

        for (int32_t i = 0, count = config_setting_length(_obj); i < count; ++i) {
            config_setting_t* s = config_setting_get_elem(_obj, i);
            if (!s)
                return rfl::Error("Error reading object");
            _object_reader.read(std::string_view{ config_setting_name(s) }, s);
        }

        return std::nullopt;
    }

    /// Constructs T using its custom constructor. This will only be triggered if
    /// T was determined to have a custom constructor by
    /// static constexpr bool has_custom_constructor, as defined above.
    /// Returns an rfl::Error, if the custom constructor throws an exception.
    template<class T>
    rfl::Result<T> use_custom_constructor(const InputVarType& _var) const noexcept
    {
        // If you do not want to support this functionality,
        // just return this.
        return rfl::Error("Not supported.");
    }
};
}
