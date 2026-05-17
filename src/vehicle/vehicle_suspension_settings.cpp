#include "vehicle_suspension_settings.h"
#include "bind_macros.h"

void LNVehicleSuspensionSettings::_bind_methods() {
    MAKE_BIND_FLOAT(LNVehicleSuspensionSettings, maximum);
    MAKE_BIND_FLOAT(LNVehicleSuspensionSettings, minimum);
    MAKE_BIND_FLOAT(LNVehicleSuspensionSettings, rest);
    MAKE_BIND_FLOAT(LNVehicleSuspensionSettings, bump);
    MAKE_BIND_FLOAT(LNVehicleSuspensionSettings, rebound);
    MAKE_BIND_FLOAT(LNVehicleSuspensionSettings, spring_rate);
}
