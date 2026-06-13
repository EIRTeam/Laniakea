#include "ln_timer.h"

#include "game/main_loop.h"

namespace LNTimer {

double LNIntervalTimer::now() const {
	switch (update_mode) {
		case LNTimerUpdateMode::PHYSICS: {
			return LaniakeaMainLoop::get_singleton()->get_physics_time();
		} break;
		case LNTimerUpdateMode::PROCESS: {
			return LaniakeaMainLoop::get_singleton()->get_process_time();
		} break;
	}
}

LNIntervalTimer::LNIntervalTimer(LNTimerUpdateMode p_update_mode) :
		update_mode(p_update_mode) {}

bool LNIntervalTimer::is_greater_than(double p_interval) const {
	return (now() - time) > p_interval;
}

void LNIntervalTimer::reset() {
	time = now();
}

void LNIntervalTimer::start() {
	time = now();
}

} //namespace LNTimer
