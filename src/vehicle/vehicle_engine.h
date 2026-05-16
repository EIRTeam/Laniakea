#pragma once

#include "bind_macros.h"
#include "vehicle/vehicle_engine_settings.h"
class LNVehicleEngine : public RefCounted {
    GDCLASS(LNVehicleEngine, RefCounted);
    Ref<LNVehicleEngineSettings> engine_settings;

    float output_torque = 0.0f;
    float angular_velocity = 0.0f;
    float time = 0.0f;
    float next_limiter_bounce = 0.0f;
public:
    MAKE_SETTER_GETTER_VALUE(Ref<LNVehicleEngineSettings>, engine_settings, engine_settings);

    void update(float p_throttle, float p_clutch_torque, double p_delta);
    void set_rpm(float p_rpm);
    float get_rpm() const;
    static void _bind_methods();
};