#pragma once

#include "bind_macros.h"
#include "godot_cpp/classes/ref_counted.hpp"
#include "vehicle/vehicle_drivetrain_config.h"

using namespace godot;

class LNVehicleDrivetrain : public RefCounted {
    GDCLASS(LNVehicleDrivetrain, RefCounted);

    Ref<LNVehicleDrivetrainSettings> drivetrain_settings;

    float current_clutch_torque = 0.0f;
    int current_gear = 0;

public:
    MAKE_SETTER_GETTER_VALUE(Ref<LNVehicleDrivetrainSettings>, drivetrain_settings, drivetrain_settings);
    void update_clutch(float p_clutch_pedal_input, float p_velocity_in_rads, float p_velocity_from_gearbox, float p_delta);
    float clutch_get_upstream_torque() const;
    float gearbox_angular_vel_to_upstream(float p_angular_vel) const;
    float gearbox_get_downstream_torque(float p_engine_torque) const;
    Pair<float, float> differential_get_downstream_torque(float p_gearbox_torque) const;
    float differential_get_upstream_angular_velocity(float p_wheel_angular_velocity_left, float p_wheel_angular_velocity_right) const;
    static void _bind_methods() {}
    void gear_up();
    void gear_down();
    int get_current_gear() const;

    void update_differential(float p_wheel_load_torque, float p_delta);
};