#pragma once

#include "bind_macros.h"
#include "godot_cpp/classes/resource.hpp"
#include "vehicle/vehicle_drivetrain_config.h"
#include "vehicle/vehicle_engine_settings.h"

using namespace godot;

class LNVehicleSettings : public Resource {
	GDCLASS(LNVehicleSettings, Resource);

	Ref<LNVehicleEngineSettings> engine_settings;
	Ref<LNVehicleDrivetrainSettings> drivetrain_settings;
	float front_arb_stiffness = 15000.0f;
	float rear_arb_stiffness = 10000.0f;

	float steer_lock = 478.8f;
	float steer_ratio = 13.6f;
	float linear_steer_rod_ratio = 0.0022f;

public:
	MAKE_SETTER_GETTER_VALUE(Ref<LNVehicleEngineSettings>, engine_settings, engine_settings);
	MAKE_SETTER_GETTER_VALUE(Ref<LNVehicleDrivetrainSettings>, drivetrain_settings, drivetrain_settings);
	MAKE_SETTER_GETTER_FLOAT_VALUE(front_arb_stiffness, front_arb_stiffness);
	MAKE_SETTER_GETTER_FLOAT_VALUE(rear_arb_stiffness, rear_arb_stiffness);
	MAKE_SETTER_GETTER_FLOAT_VALUE(steer_lock, steer_lock);
	MAKE_SETTER_GETTER_FLOAT_VALUE(steer_ratio, steer_ratio);
	MAKE_SETTER_GETTER_FLOAT_VALUE(linear_steer_rod_ratio, linear_steer_rod_ratio);
	static void _bind_methods();
};
