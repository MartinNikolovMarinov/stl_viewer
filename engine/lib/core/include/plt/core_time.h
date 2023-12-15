#pragma once

#include <core_API.h>
#include <core_types.h>
#include <core_expected.h>
#include <plt/core_plt_error.h>

namespace core {

using namespace coretypes;

CORE_API_EXPORT expected<u64, PltErrCode> getCurrentUnixTimestampMs();

} // namespace core
