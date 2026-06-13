#include "vehicle_settings.h"

void LNVehicleSettings::_bind_methods() {
	MAKE_BIND_RESOURCE(LNVehicleSettings, engine_settings, LNVehicleEngineSettings);
	MAKE_BIND_RESOURCE(LNVehicleSettings, drivetrain_settings, LNVehicleDrivetrainSettings);
	MAKE_BIND_FLOAT(LNVehicleSettings, front_arb_stiffness);
	MAKE_BIND_FLOAT(LNVehicleSettings, rear_arb_stiffness);
	MAKE_BIND_FLOAT(LNVehicleSettings, steer_lock);
	MAKE_BIND_FLOAT(LNVehicleSettings, steer_ratio);
	MAKE_BIND_FLOAT(LNVehicleSettings, linear_steer_rod_ratio);
}
