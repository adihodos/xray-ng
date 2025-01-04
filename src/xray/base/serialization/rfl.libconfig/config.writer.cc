#include "xray/base/serialization/rfl.libconfig/config.writer.hpp"
#include <fmt/core.h>
#include <fmt/format.h>

namespace rfl::libconfig {

Writer::Writer(config_t* doc)
    : doc_{ doc }
{
}

Writer::OutputArrayType
Writer::array_as_root(const size_t) const noexcept
{
    char temp_buff[512];
    auto out = fmt::format_to_n(temp_buff, std::size(temp_buff) - 1, "array_{}", count_array_++);
    *out.out = 0;
    config_setting_t* s = config_setting_add(config_root_setting(doc_), temp_buff, CONFIG_TYPE_LIST);
    return LibconfigOutputArray{ s };
}

Writer::OutputObjectType
Writer::object_as_root(const size_t) const noexcept
{
    return OutputObjectType{ config_root_setting(doc_) };
};

Writer::OutputVarType
Writer::null_as_root() const noexcept
{
    config_setting_t* r = config_root_setting(doc_);
    return OutputVarType{ r };
}

Writer::OutputArrayType
Writer::add_array_to_array(const size_t, OutputArrayType* _parent) const noexcept
{
    config_setting_t* arr = config_setting_add(_parent->val_, nullptr, CONFIG_TYPE_LIST);
    return OutputArrayType{ arr };
}

Writer::OutputArrayType
Writer::add_array_to_object(const std::string_view& _name, const size_t, OutputObjectType* _parent) const noexcept
{
    config_setting_t* arr = config_setting_add(_parent->val_, _name.data(), CONFIG_TYPE_LIST);
    return OutputArrayType{ arr };
}

Writer::OutputObjectType
Writer::add_object_to_array(const size_t, OutputArrayType* _parent) const noexcept
{
    config_setting_t* obj = config_setting_add(_parent->val_, nullptr, CONFIG_TYPE_GROUP);
    return OutputObjectType{ obj };
}

Writer::OutputObjectType
Writer::add_object_to_object(const std::string_view& _name, const size_t, OutputObjectType* _parent) const noexcept
{
    config_setting_t* obj = config_setting_add(_parent->val_, _name.data(), CONFIG_TYPE_GROUP);
    return OutputObjectType{ obj };
}

}
