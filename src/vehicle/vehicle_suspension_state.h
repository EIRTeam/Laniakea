#pragma once

#include "godot_cpp/variant/transform3d.hpp"

using namespace godot;

struct LNVehicleSuspensionState {
	float prev_shock_length = 0.0f;
	float shock_length = 0.0f;
	Transform3D suspension_transform_world;
	float steering_angle_rads = 0.0f;
	bool can_steer = false;

	virtual ~LNVehicleSuspensionState() = default;
};
