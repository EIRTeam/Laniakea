#include "vehicle_wheel_settings.h"

#include "bind_macros.h"

void LNVehicleWheelSettings::_bind_methods() {
	MAKE_BIND_FLOAT(LNVehicleWheelSettings, mass);
	MAKE_BIND_RESOURCE(LNVehicleWheelSettings, tyre, LNVehicleTyre);
}
