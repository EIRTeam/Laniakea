#pragma once

#ifdef TRACY_ENABLE

#include "tracy/Tracy.hpp"

#define FuncProfile ZoneScopedN(__PRETTY_FUNCTION__)
#else
#define FuncProfile
#endif
