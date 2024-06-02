#pragma once

namespace xray::base {

template<typename... Visitors>
struct VariantVisitor : Visitors...
{
    using Visitors::operator()...;
};

} // namespace xray::base
