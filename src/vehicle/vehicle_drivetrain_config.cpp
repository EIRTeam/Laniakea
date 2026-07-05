#include "vehicle_drivetrain_config.h"

#include "bind_macros.h"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/core/error_macros.hpp"

void LNVehicleDrivetrainSettings::_bind_methods() {
	MAKE_BIND_FLOAT32_ARRAY(LNVehicleDrivetrainSettings, positive_gearbox_ratios);
	MAKE_BIND_FLOAT32_ARRAY(LNVehicleDrivetrainSettings, negative_gearbox_ratios);
	MAKE_BIND_FLOAT(LNVehicleDrivetrainSettings, gearbox_inertia);
	MAKE_BIND_FLOAT(LNVehicleDrivetrainSettings, clutch_max_torque);
	MAKE_BIND_FLOAT(LNVehicleDrivetrainSettings, clutch_lock_threshold);
	MAKE_BIND_FLOAT(LNVehicleDrivetrainSettings, clutch_response_rate);
	MAKE_BIND_FLOAT(LNVehicleDrivetrainSettings, differential_inertia);

	BIND_ENUM_CONSTANT(DifferentialType::LOCKED);
	BIND_ENUM_CONSTANT(DifferentialType::OPEN);
}

float LNVehicleDrivetrainSettings::get_gear_ratio(int p_gear) const {
	if (p_gear == 0) {
		return 0.0f;
	}
	if (p_gear > 0) {
		ERR_FAIL_INDEX_V(p_gear - 1, positive_gearbox_ratios.size(), 0.0f);
		return positive_gearbox_ratios[p_gear - 1];
	}

	ERR_FAIL_INDEX_V(-p_gear - 1, negative_gearbox_ratios.size(), 0.0f);
	return negative_gearbox_ratios[-p_gear - 1];
}
