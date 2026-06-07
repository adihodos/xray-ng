#pragma once

namespace xray::build::config {
static inline constexpr const char* const Xray_CommitHash_Short = XRAY_BUILD_GIT_HASH;
static inline constexpr const char* const Xray_CommitHash_Full	= XRAY_BUILD_GIT_HASH_FULL;
static inline constexpr const char* const Xray_UserName			= XRAY_USER_NAME;
static inline constexpr const char* const Xray_MachineName		= XRAY_MACHINE_NAME;
}  // namespace xray::build::config
