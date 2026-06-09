#include "vehicle_suspension_settings.h"
#include "bind_macros.h"

void LNVehicleSuspensionSettings::_bind_methods() {
    MAKE_BIND_FLOAT(LNVehicleSuspensionSettings, bumpstop_up);
    MAKE_BIND_FLOAT(LNVehicleSuspensionSettings, bumpstop_down);
    MAKE_BIND_FLOAT(LNVehicleSuspensionSettings, packer_range);

    MAKE_BIND_FLOAT(LNVehicleSuspensionSettings, rod_length_offset);

    MAKE_BIND_FLOAT(LNVehicleSuspensionSettings, bumpstop_rate);
    MAKE_BIND_FLOAT(LNVehicleSuspensionSettings, spring_rate);

    MAKE_BIND_FLOAT(LNVehicleSuspensionSettings, bump_damp_rate);
    MAKE_BIND_FLOAT(LNVehicleSuspensionSettings, bump_fast_damp_rate);
    MAKE_BIND_FLOAT(LNVehicleSuspensionSettings, bump_fast_damp_threshold);
    
    MAKE_BIND_FLOAT(LNVehicleSuspensionSettings, rebound_damp_rate);
    MAKE_BIND_FLOAT(LNVehicleSuspensionSettings, rebound_fast_damp_rate);
    MAKE_BIND_FLOAT(LNVehicleSuspensionSettings, rebound_fast_damp_rate_threshold);

    MAKE_BIND_FLOAT(LNVehicleSuspensionSettings, toe_out);
    MAKE_BIND_FLOAT(LNVehicleSuspensionSettings, static_camber_degrees);

    MAKE_BIND_VECTOR3(LNVehicleSuspensionSettings, strut_car);
}
