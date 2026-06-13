#pragma once

#include "godot_cpp/variant/vector3.hpp"
#include "vehicle/vehicle_settings.h"
#include "vehicle/vehicle_suspension_settings.h"

using namespace godot;

namespace LNVehicleSteeringRack {
struct SteeringRackResult {
	Vector3 left_position;
	Vector3 right_position;
};

static SteeringRackResult solve(Ref<LNVehicleSettings> p_vehicle_settings, std::array<Ref<LNVehicleSuspensionSettings>, 2> p_suspension_settings, std::array<Transform3D, 2> p_suspension_transforms, float p_steer_amount) {
	const float steer_offset = ((p_steer_amount * p_vehicle_settings->get_steer_lock()) / p_vehicle_settings->get_steer_ratio()) * p_vehicle_settings->get_linear_steer_rod_ratio();
	const Vector3 steering_rack_left_end = p_suspension_transforms[0].xform(p_suspension_settings[0]->get_steering_rod_rack());
	const Vector3 steering_rack_right_end = p_suspension_transforms[1].xform(p_suspension_settings[1]->get_steering_rod_rack());
	const Vector3 movement_dir = steering_rack_left_end.direction_to(steering_rack_right_end);

	return {
		.left_position = steering_rack_left_end + movement_dir * steer_offset,
		.right_position = steering_rack_right_end + movement_dir * steer_offset,
	};
}
} //namespace LNVehicleSteeringRack
