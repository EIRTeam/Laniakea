#pragma once

#include "bind_macros.h"
#include "godot_cpp/classes/audio_stream_player.hpp"
#include "godot_cpp/classes/rigid_body3d.hpp"
#include "../console/cvar.h"
#include "godot_cpp/core/binder_common.hpp"
#include "vehicle/vehicle_drivetrain.h"
#include "vehicle/vehicle_engine.h"
#include "vehicle/vehicle_settings.h"

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
        float angle = 0.0f;
        float drive_torque = 0.0f;
        float brake_torque = 0.0f;
        float slip_ratio = 0.0f;
        float slip_angle = 0.0f;
        float differential_tan_slip_angle = 0.0f;
        float differential_slip_ratio = 0.0f;
        Vector3 contact_normal;
        float longitudinal_torque = 0.0f;
    };

    struct Input {
        float brake_percentage = 0.0f;
        float steer = 0.0f;
        float throttle = 0.0f;
        float clutch = 0.0f;
        int gear = 0;
    } input;
    void _apply_arb(int p_wheel_left, int p_wheel_right, float p_arb_stiffness);
    AudioStreamPlayer *audio_stream_player = nullptr;

    MAKE_SETTER_GETTER_VALUE(AudioStreamPlayer*, audio_stream_player, audio_stream_player)

    std::array<WheelData, static_cast<size_t>(LNVehicleWheelPosition::WHEEL_MAX)> wheels = {};

    CVar *vehicle_draw_wheels_cvar;

    Ref<LNVehicleEngine> engine;
    Ref<LNVehicleDrivetrain> drivetrain;
    Ref<LNVehicleSettings> vehicle_settings;

    MAKE_SETTER_GETTER_VALUE(Ref<LNVehicleSettings>, vehicle_settings, vehicle_settings);

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
    void set_throttle_percentage(float p_throttle_percentage);
    void set_clutch_percentage(float p_clutch_precentage);
    void request_gear_up();
    void request_gear_down();
    int get_current_gear() const;
    float get_wheel_slip_angle(LNVehicleWheelPosition p_wheel) const;
    float get_wheel_slip_ratio(LNVehicleWheelPosition p_wheel) const;
    float get_engine_torque() const;
    float get_engine_rpm() const;
    virtual void _ready() override;
    LNVehicle();
};

VARIANT_ENUM_CAST(LNVehicleWheelPosition);