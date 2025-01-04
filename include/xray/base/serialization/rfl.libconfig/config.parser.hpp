#pragma once

#include <rfl/parsing/Parser.hpp>
#include "xray/base/serialization/rfl.libconfig/config.reader.hpp"
#include "xray/base/serialization/rfl.libconfig/config.writer.hpp"

namespace rfl::libconfig {

template<class T, class ProcessorsType>
using Parser = parsing::Parser<Reader, Writer, T, ProcessorsType>;

} // namespace rfl::toml
