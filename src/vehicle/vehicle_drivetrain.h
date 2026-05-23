#pragma once

#include "bind_macros.h"
#include "godot_cpp/classes/ref_counted.hpp"
#include "vehicle/shaft.h"
#include "vehicle/vehicle_differential_settings.h"
#include "vehicle/vehicle_drivetrain_config.h"
#include "vehicle/vehicle_engine.h"

using namespace godot;

class LNVehicleDrivetrain : public RefCounted {
    GDCLASS(LNVehicleDrivetrain, RefCounted);

    Ref<LNVehicleDrivetrainSettings> drivetrain_settings;

    float current_clutch_torque = 0.0f;
    int current_gear = 0;
    float gearbox_input_shaft_angular_vel = 0.0f;
    float differential_angular_velocity = 0.0f;

    struct Clutch {
        bool locked = false;
    } clutch;

public:
    MAKE_SETTER_GETTER_VALUE(Ref<LNVehicleDrivetrainSettings>, drivetrain_settings, drivetrain_settings);
    float gearbox_get_downstream_angular_velocity() const;
    float gearbox_get_downstream_torque() const;
    Pair<float, float> differential_get_downstream_torque(float p_gearbox_torque) const;
    static void _bind_methods() {}
    void gear_up();
    void gear_down();
    int get_current_gear() const;

    void update(Ref<LNVehicleEngine> p_engine, float p_throttle, float p_clutch_pedal_input, float p_wheel_angular_velocity_left, float p_wheel_angular_velocity_right, float p_wheel_inertia, float p_delta);

    void gearbox_get_upstream_angular_velocity_and_inertia(float &r_upstream_angular_vel, float &r_upstream_inertia, float p_velocity_downstream, float p_inertia_downstream);
    void diff_get_upstream_angular_velocity_and_inertia(float &r_upstream_angular_vel, float &r_upstream_inertia, float p_velocity_downstream_left, float p_velocity_downstream_right, float p_inertia_downstream_left, float p_inertia_downstream_right);

    void gearbox_get_downstream_inertia(
        float &r_downstream_inertia, 
        float p_inertia_upstream);

    void diff_get_downstream_inertia(
    float &r_down_inertia_left, float &r_down_inertia_right,
    float p_inertia_upstream);

    Vector2 GetDownstreamTorque(float p_gearbox_torque, Vector2 p_wheel_angular_velocities, Vector2 p_extra_torques, Vector2 p_wheel_inertias, float p_delta);
};