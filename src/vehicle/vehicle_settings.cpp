#include "vehicle_settings.h"

void LNVehicleSettings::_bind_methods() {
    MAKE_BIND_RESOURCE(LNVehicleSettings, engine_settings, LNVehicleEngineSettings);
    MAKE_BIND_RESOURCE(LNVehicleSettings, drivetrain_settings, LNVehicleDrivetrainSettings);
}
