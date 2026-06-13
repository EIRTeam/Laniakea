#pragma once

#include "debug/debug_overlay.h"
#include "godot_cpp/classes/physics_direct_space_state3d.hpp"
#include "godot_cpp/classes/physics_ray_query_parameters3d.hpp"
#include "godot_cpp/classes/world3d.hpp"
#include "godot_cpp/variant/vector3.hpp"
#include "vehicle/vehicle.h"

using namespace godot;

namespace LNVehicleSuspension {
struct SuspensionForceResult {
	float compression = 0.0f;

	// Positive = rebound = length increase
	// Negative = bump = length decrease
	float velocity = 0.0f;

	float spring_force = 0.0f;
	float damping_force = 0.0f;

	float total_force = 0.0f;
	float clamped_total_force = 0.0f;
};

SuspensionForceResult compute_suspension_force(Ref<LNVehicleSuspensionSettings> p_suspension_settings, const bool p_is_grounded, const float p_rest_spring_length, const float p_prev_length, const float p_spring_length, const float p_delta);

struct WheelFrameOut {
	Vector3 steering_axis_up_world;
	Vector3 steering_axis_right_world;
	Vector3 steering_axis_forward_world;
	Vector3 steering_rod_hub;
};

struct WheelFrameComputationParams {
	// Position of the tie rod ball joint in the steering rack
	Vector3 tie_rod_rack_position_world;
	// Tie rod position in wheel space
	Vector3 tie_rod_wheel_local;
	float tie_rod_length = 0.0f;

	Vector3 bottom_ball_joint_world;

	// Camber at design 0
	float design_zero_camber = 0.0f;

	// toe at design 0
	float design_zero_toe = 0.0f;

	Transform3D vehicle_transform;

	float side_sign = 1.0f;
	float setup_camber = 0.0f;
	float setup_toe = 0.0f;
	Vector3 steering_axis_world;

	bool use_steering = true;
};

WheelFrameOut build_wheel_frame(const WheelFrameComputationParams &p_wheel_frame_params);

struct WheelIntersectionResult {
	bool hit = false;
	Vector3 wheel_center_position;
	Vector3 ground_hit_position;
	Vector3 ground_normal;
};

WheelIntersectionResult do_wheel_intersection(const Node3D *p_vehicle, Vector3 p_wheel_right, Vector3 p_wheel_up, float p_wheel_width, float p_wheel_radius, Vector3 p_wheel_center, float p_muf_width, float p_muf_long);
}; //namespace LNVehicleSuspension
