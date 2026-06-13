#pragma once

#include "godot_cpp/core/defs.hpp"

#include <cstdint>
namespace LNTimer {
enum class LNTimerUpdateMode {
	PHYSICS,
	PROCESS
};

enum class LNTimerState : uint8_t {
	TICKING,
	STOPPED
};

class LNIntervalTimer {
	LNTimerUpdateMode update_mode;

	double time = 0.0f;

	double now() const;

public:
	LNIntervalTimer(LNTimerUpdateMode p_update_mode);
	bool is_greater_than(double p_time) const;
	void reset();
	void start();
};
} //namespace LNTimer
