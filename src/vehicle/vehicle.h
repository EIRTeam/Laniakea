#pragma once

#include "godot_cpp/classes/rigid_body3d.hpp"
#include "../console/cvar.h"
#include "godot_cpp/core/binder_common.hpp"

using namespace godot;

class LNVehicleWheel;

enum LNVehicleWheelPosition {
    WHEEL_FL,
    WHEEL_FR,
    WHEEL_RL,
    WHEEL_RR,
    WHEEL_MAX
};

class LNVehicle : public RigidBody3D {
    GDCLASS(LNVehicle, RigidBody3D);
public:
    struct WheelData {
        LNVehicleWheel *wheel = nullptr;
        float spring_displacement = 0.0f; // Positive is extended, negative is compressed
        bool hit = false;
        Vector3 hit_position;
        float tire_force_longitudinal = 0.0f;
        float tire_force_lateral = 0.0f;
        float spring_force = 0.0f;
        float spring_velocity = 0.0f;
        float angular_velocity = 0.0f;
        float drive_torque = 0.0f;
        float brake_torque = 0.0f;
        float slip_ratio = 0.0f;
        float slip_angle = 0.0f;
    };

    struct Input {
        float brake_percentage = 0.0f;
        float steer = 0.0f;
    } input;

    std::array<WheelData, static_cast<size_t>(LNVehicleWheelPosition::WHEEL_MAX)> wheels = {};

    CVar *vehicle_draw_wheels_cvar;

    void _apply_force(Vector3 p_force_global, Vector3 p_offset_global);

    static void _bind_methods();
    virtual void _physics_process(double p_delta) override;
    void _process_wheel_grounded(WheelData &p_wheel, const Vector3 &p_world_wheel_direction, float p_delta);
    void _process_wheel_airborne(WheelData &p_wheel, const Vector3 &p_world_wheel_direction, const Vector3 &p_world_attachment_point, float p_delta);
    void _debug_draw();

    Vector3 wheel_get_world_forward(LNVehicleWheel *p_wheel) const;
    Vector3 wheel_get_world_right(LNVehicleWheel *p_wheel) const;

    void register_wheel(LNVehicleWheel *p_wheel);
    void unregister_wheel(LNVehicleWheel *p_wheel);
    void _initialize();
    void set_brake_percentage(float p_brake_percentage);
    void set_steer_percentage(float p_steer_percentage);
    LNVehicle();
};

VARIANT_ENUM_CAST(LNVehicleWheelPosition);