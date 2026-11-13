#pragma once
#include <stdint.h>

namespace ck::arm {

using NativeImport = uint64_t (*)(const uint32_t*);

NativeImport FindImport(const char* name);

} // namespace ck::arm
