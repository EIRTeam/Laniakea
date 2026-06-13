#include "movement_settings.h"

#include "bind_macros.h"
#include "game/movement_shared.h"
#include "godot_cpp/core/error_macros.hpp"

void MovementSettings::_bind_methods() {
	MAKE_BIND_FLOAT(MovementSettings, crouching_acceleration);
	MAKE_BIND_FLOAT(MovementSettings, standing_acceleration);
	MAKE_BIND_FLOAT(MovementSettings, crouching_max_walk_speed);
	MAKE_BIND_FLOAT(MovementSettings, standing_max_walk_speed);
	MAKE_BIND_FLOAT(MovementSettings, crouching_max_run_speed);
	MAKE_BIND_FLOAT(MovementSettings, standing_max_run_speed);
	MAKE_BIND_FLOAT(MovementSettings, crouching_max_sprint_speed);
	MAKE_BIND_FLOAT(MovementSettings, standing_max_sprint_speed);
	MAKE_BIND_FLOAT(MovementSettings, crouching_height);
	MAKE_BIND_FLOAT(MovementSettings, standing_height);
	MAKE_BIND_FLOAT(MovementSettings, radius);
	MAKE_BIND_FLOAT(MovementSettings, max_slope_angle);
	MAKE_BIND_FLOAT(MovementSettings, max_step_height);
	MAKE_BIND_FLOAT(MovementSettings, min_step_depth);
	MAKE_BIND_FLOAT(MovementSettings, snap_to_ground_height);
	MAKE_BIND_FLOAT(MovementSettings, terminal_velocity);
	MAKE_BIND_FLOAT(MovementSettings, gravity);
	MAKE_BIND_FLOAT(MovementSettings, push_mass);
	MAKE_BIND_FLOAT(MovementSettings, movement_halflife);
}

float MovementSettings::get_stance_height(int p_stance) const {
	ERR_FAIL_INDEX_V(p_stance, Movement::STANCE_MAX, 0.0f);
	return height[p_stance];
}
float MovementSettings::get_stance_max_walk_speed(int p_stance) const {
	ERR_FAIL_INDEX_V(p_stance, Movement::STANCE_MAX, 0.0f);
	return max_walk_speed[p_stance];
}

float MovementSettings::get_stance_max_run_speed(int p_stance) const {
	ERR_FAIL_INDEX_V(p_stance, Movement::STANCE_MAX, 0.0f);
	return max_run_speed[p_stance];
}

float MovementSettings::get_stance_max_sprint_speed(int p_stance) const {
	ERR_FAIL_INDEX_V(p_stance, Movement::STANCE_MAX, 0.0f);
	return max_sprint_speed[p_stance];
}
