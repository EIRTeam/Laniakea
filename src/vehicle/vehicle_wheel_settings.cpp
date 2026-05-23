#include "vehicle_wheel_settings.h"
#include "bind_macros.h"

void LNVehicleWheelSettings::_bind_methods() {
    MAKE_BIND_FLOAT(LNVehicleWheelSettings, mass);
    MAKE_BIND_FLOAT(LNVehicleWheelSettings, radius);
    MAKE_BIND_FLOAT(LNVehicleWheelSettings, width);
    MAKE_BIND_FLOAT(LNVehicleWheelSettings, stiffness);
    MAKE_BIND_FLOAT(LNVehicleWheelSettings, contact_patch);
    MAKE_BIND_FLOAT(LNVehicleWheelSettings, coefficient_of_friction);
}
