#pragma once

#include "debug/debug_overlay.h"
#include "godot_cpp/classes/geometry3d.hpp"
#include "godot_cpp/classes/input.hpp"
#include "godot_cpp/classes/node3d.hpp"
#include "godot_cpp/classes/physics_direct_space_state3d.hpp"
#include "godot_cpp/classes/physics_ray_query_parameters3d.hpp"
#include "godot_cpp/classes/world3d.hpp"
#include "godot_cpp/core/math.hpp"
#include "godot_cpp/variant/plane.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "math.h"
#include "vehicle/steering_rack.h"
#include "vehicle/tire_model_lastminute.h"
#include "vehicle/vehicle_settings.h"
#include "vehicle/vehicle_suspension.h"
#include "vehicle/vehicle_suspension_macpherson_settings.h"
#include "vehicle/vehicle_suspension_settings.h"
#include "vehicle/vehicle_suspension_state.h"
#include "vehicle/vehicle_wheel_settings.h"
#include "vehicle/wheel_position.h"

using namespace godot;
class SuspensionTest : public Node3D {
	GDCLASS(SuspensionTest, Node3D);

public:
	LNVehicleSuspensionState *state_left;
	LNVehicleSuspensionState *state_right;
	Ref<LNVehicleMacPhersonSuspensionSettings> settings;
	Ref<LNVehicleWheelSettings> wheel_settings;
	Ref<LNVehicleSettings> vehicle_settings;
	float steer = 0.0f;

	struct SteeringRackSettings {
		float steer_lock = 478.8f;
		float steer_ratio = 13.6f;
		float linear_steer_rod_ratio = 0.0022f;
	};

	SuspensionTest() {
		settings.instantiate();
		settings->set_bottom_wishbone_front(Vector3(0.438, -0.16775, -0.26073));
		settings->set_bottom_wishbone_rear(Vector3(0.41057, -0.15672, 0.0128));
		settings->set_bottom_wishbone_tyre(Vector3(0.10784, -0.16402, -0.01798));
		settings->set_strut_car(Vector3(0.28497, 0.40218001, 0.08294));
		settings->set_steering_rod_rack(Vector3(0.48843, -0.09289, -0.10865));
		settings->set_steering_rod_hub(Vector3(0.09707, -0.08479, -0.14781));
		settings->set_static_camber_degrees(-0.0f);
		state_left = settings->create_suspension_state();
		state_right = settings->create_suspension_state();
		wheel_settings.instantiate();
		Ref<LNVehicleTyreLastMinute> tyre_model;
		tyre_model.instantiate();
		wheel_settings->set_tyre(tyre_model);
		tyre_model->set_radius(0.3f);

		vehicle_settings.instantiate();
	}

	SteeringRackSettings rack_settings;

	virtual void _physics_process(double p_delta) override {
		// Simulate steer
		float steer_offset = ((steer * rack_settings.steer_lock) / rack_settings.steer_ratio) * rack_settings.linear_steer_rod_ratio;

		print_line(steer_offset);

		const float steering_target = Input::get_singleton()->get_action_strength("steer_right") - Input::get_singleton()->get_action_strength("steer_left");

		steer = Math::move_toward(steer, steering_target, (float)Math_PI * (float)p_delta);

		state_left->steering_angle_rads = steer;
		state_right->steering_angle_rads = steer;
		state_left->can_steer = true;
		state_right->can_steer = true;

		set_global_basis(Basis().rotated(Vector3(0.0, 1.0, 0.0), Math::deg_to_rad(0.0f)));
		Transform3D trf_left;
		trf_left.origin.x = -1.475 / 2.0f;
		Transform3D trf_right;
		trf_right.origin.x = 1.475 / 2.0f;
		trf_right.basis.scale_local(Vector3(-1.0f, 1.0f, 1.0f));

		LNVehicleSteeringRack::SteeringRackResult steering_result = LNVehicleSteeringRack::solve(vehicle_settings, { settings, settings }, { trf_left, trf_right }, steer);

		_solve_macpherson(trf_left, steering_result.left_position, WHEEL_FL, p_delta, reinterpret_cast<LNVehicleMacPhersonSuspensionSettings::LNVehicleMacPhersonSuspensionState *>(state_left));
		_solve_macpherson(trf_right, steering_result.right_position, WHEEL_FR, p_delta, reinterpret_cast<LNVehicleMacPhersonSuspensionSettings::LNVehicleMacPhersonSuspensionState *>(state_right));

		DebugOverlay::line(steering_result.left_position, steering_result.right_position, Color("RED"));
	}
	virtual void _solve_macpherson(const Transform3D &p_suspension_transform, Vector3 p_tie_rod_rack_world, const LNVehicleWheelPosition p_wheel_pos, double p_delta, LNVehicleMacPhersonSuspensionSettings::LNVehicleMacPhersonSuspensionState *p_state) {
		p_state->suspension_transform_world = p_suspension_transform * get_global_transform();
		settings->solve(wheel_settings, p_tie_rod_rack_world, p_wheel_pos, this, p_delta, p_state);
	}

	static void _bind_methods() {}
};
