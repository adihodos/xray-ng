#pragma once

#include <type_traits>
#include <swl/variant.hpp>

namespace xray::base {

template<typename... Visitors>
struct VariantVisitor : Visitors...
{
    using Visitors::operator()...;
};

template<typename... Out, typename... In>
auto
variant_transform(const swl::variant<In...>& invar) -> swl::variant<Out...>
{
    return swl::visit(
        [](const auto& value) {
            return swl::variant<Out...>{ swl::in_place_type<std::remove_cvref_t<decltype(value)>>, value };
        },
        invar);
};

struct DetectVariant
{
    template<typename... Vars>
    static constexpr std::true_type func(const swl::variant<Vars...>&) noexcept;

    template<typename U>
    static constexpr std::false_type func(const U&) noexcept;
};

template<typename T>
constexpr bool is_swl_variant_v = std::is_same_v<std::true_type, decltype(DetectVariant::func(std::declval<T>()))>;

} // namespace xray::base
