#pragma once

#include "bind_macros.h"
#include "godot_cpp/classes/resource.hpp"
#include "godot_cpp/variant/packed_float32_array.hpp"

using namespace godot;

class LNVehicleDrivetrainSettings : public Resource {
	GDCLASS(LNVehicleDrivetrainSettings, Resource);

	float gearbox_inertia = 0.02f;
	float differential_inertia = 0.02f;
	PackedFloat32Array negative_gearbox_ratios;
	PackedFloat32Array positive_gearbox_ratios;

	float clutch_max_torque = 400.0f;
	float clutch_response_rate = 15.0f;
	float clutch_lock_threshold = 0.5f;
	float final_ratio = 4.0f;
	float autoclutch_min = 1200.0f;
	float autoclutch_max = 1800.0f;

public:
	MAKE_SETTER_GETTER_FLOAT_VALUE(clutch_max_torque, clutch_max_torque);
	MAKE_SETTER_GETTER_FLOAT_VALUE(clutch_response_rate, clutch_response_rate);
	MAKE_SETTER_GETTER_FLOAT_VALUE(clutch_lock_threshold, clutch_lock_threshold);
	MAKE_SETTER_GETTER_FLOAT_VALUE(gearbox_inertia, gearbox_inertia);
	MAKE_SETTER_GETTER_FLOAT_VALUE(differential_inertia, differential_inertia);
	MAKE_SETTER_GETTER_FLOAT_VALUE(final_ratio, final_ratio);
	MAKE_SETTER_GETTER_VALUE(PackedFloat32Array, negative_gearbox_ratios, negative_gearbox_ratios);
	MAKE_SETTER_GETTER_VALUE(PackedFloat32Array, positive_gearbox_ratios, positive_gearbox_ratios);
	MAKE_SETTER_GETTER_FLOAT_VALUE(autoclutch_min, autoclutch_min);
	MAKE_SETTER_GETTER_FLOAT_VALUE(autoclutch_max, autoclutch_max);
	static void _bind_methods();

	float get_gear_ratio(int p_gear) const;
};
