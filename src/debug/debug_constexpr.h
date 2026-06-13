#pragma once

namespace Debug {
#ifdef DEBUG_ENABLED
static constexpr bool is_debug_enabled = true;
#else
static constexpr bool is_debug_enabled = false;
#endif
} //namespace Debug
