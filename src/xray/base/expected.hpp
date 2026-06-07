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

/// \file maybe.hpp

#include "xray/xray.hpp"
#include "xray/base/xray.debug.hpp"
#include "xray/base/minstd/fwd.move.hpp"
#include "xray/base/minstd/declval.hpp"
#include "xray/base/xray.stringview.hpp"

namespace xray::base {

/// \addtogroup __GroupXrayBase
/// @{

struct ErrorLocationInfo {
	xray::base::xrStringView_t file_loc;
	xray::base::xrStringView_t func_name;
	xray::I32 file_line;
};

struct ExpectedInPlaceTag {};

/// Simple std::expected clone (WIP)
template <typename value_type, typename error_type>
class Expected {
	/// \name Defined types.
	/// @{

public:
	using class_type = Expected<value_type, error_type>;

	/// @}

	/// \name Constructors
	/// @{

public:
	constexpr Expected(error_type&& e) noexcept : _has_value{false} {
		new (&this->_inner._err_value) error_type{XRAY_MOVE(e)};
	}

	constexpr Expected(const error_type& e) noexcept : _has_value{false} {
		new (&this->_inner._err_value) error_type{e};
	}

	/// Construct from existing value.
	constexpr Expected(const value_type& existing_value) noexcept : _has_value{true} {
		new (&_inner._result_value) value_type{existing_value};
	}

	/// Construct from a temporary value.
	constexpr Expected(value_type&& other_val) : _has_value{true} {
		new (&_inner._result_value) value_type{XRAY_MOVE(other_val)};
	}

	template <typename... ResultArgs>
	constexpr Expected(ExpectedInPlaceTag, ResultArgs&&... result_args) noexcept : _has_value{true} {
		new (&_inner._result_value) value_type{XRAY_FWD(result_args)...};
	}

	constexpr Expected(const Expected<value_type, error_type>& rhs) : _has_value{rhs._has_value} {
		if (rhs.has_value()) {
			new (&this->_inner._result_value) value_type{rhs.value()};
		} else {
			new (&this->_inner._err_value) error_type{rhs.error()};
		}
	}

	constexpr Expected(Expected<value_type, error_type>&& rhs) noexcept : _has_value{rhs._has_value} {
		if (rhs._has_value) {
			new (&this->_inner._result_value) value_type{XRAY_MOVE(rhs._inner._result_value)};
		} else {
			new (&this->_inner._err_value) error_type{XRAY_MOVE(rhs._inner._err_value)};
		}
	}

	//
	// TODO: add assignment if needed

	template <typename F>
	constexpr Expected<value_type, error_type>& map(F&& f) & {
		if (has_value()) {
			f(this->value());
		}
		return *this;
	}

	template <typename F>
	constexpr const Expected<value_type, error_type>& map(F&& f) const& {
		if (has_value()) {
			f(this->value());
		}
		return *this;
	}

	template <typename F>
	constexpr Expected<value_type, error_type> map(F&& f) && {
		if (has_value()) {
			return Expected<value_type, error_type>{f(XRAY_MOVE(this->value()))};
		}
		return Expected<value_type, error_type>{XRAY_MOVE(this->error())};
	}

	template <typename F>
	constexpr auto map_error(F&& f) & {
		using mapped_err_type = decltype(f(minstd::declval<error_type>()));
		if (has_value()) {
			return Expected<value_type, mapped_err_type>{this->value()};
		}
		return Expected<value_type, mapped_err_type>{f(this->error())};
	}

	template <typename F>
	constexpr auto map_error(F&& f) && {
		using mapped_err_type = decltype(f(minstd::declval<error_type>()));
		if (has_value()) {
			return Expected<value_type, mapped_err_type>{XRAY_MOVE(this->value())};
		}
		return Expected<value_type, mapped_err_type>{f(XRAY_MOVE(this->error()))};
	}

	~Expected() noexcept {
		if (has_value()) {
			this->_inner._result_value.~value_type();
		} else {
			this->_inner._err_value.~error_type();
		}
	}

	/// @}

	/// \name State/sanity.
	/// @{

public:
	/// Test if object holds a valid value.
	explicit operator bool() const noexcept { return has_value(); }

	/// Test if object holds a valid value.
	bool has_value() const noexcept { return _has_value; }

	/// @}

	/// \name Stored value access.
	/// @{

public:
	/// Access the stored value. Check if the object stored a valid value first.
	value_type& value() noexcept {
		XRAY_ASSERT_NOMSG(has_value());
		return this->_inner._result_value;
	}

	/// Access the stored value. Check if the object stored a valid value first.
	const value_type& value() const noexcept {
		XRAY_ASSERT_NOMSG(has_value());
		return this->_inner._result_value;
	}

	const error_type& error() const noexcept {
		XRAY_ASSERT_NOMSG(!has_value());
		return this->_inner._err_value;
	}

	error_type& error() noexcept {
		XRAY_ASSERT_NOMSG(!has_value());
		return this->_inner._err_value;
	}

	/// @}

	/// \name Data members.
	/// @{

private:
	bool _has_value;

	union ExpectedInnerType {
		U8 _dummy;
		value_type _result_value;
		error_type _err_value;

		ExpectedInnerType() : _dummy{0} {}
		~ExpectedInnerType() {}

	} _inner;

	/// @}

	/// \name Deleted member functions.
	/// @{

private:
	/// @}
};

/// @}

}  // namespace xray::base

#define XRAY_MARK_ERROR_LOCATION() \
	xray::base::ErrorLocationInfo { .file_loc = __FILE__, .func_name = XRAY_FUNCTION_NAME, .file_line = __LINE__ }
