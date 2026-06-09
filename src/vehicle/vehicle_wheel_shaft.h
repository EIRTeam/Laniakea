#pragma once

#include "vehicle/shaft.h"
#include "vehicle/vehicle_suspension_state.h"
#include "vehicle/vehicle_wheel_settings.h"
#include "vehicle_suspension_settings.h"
#include <optional>

class LNVehicle;
class LNVehicleWheel;

class LNVehicleWheelShaft : public LNVehicleShaft {
    GDCLASS(LNVehicleWheelShaft, LNVehicleShaft);

    float drive_torque = 0.0f;
    float drive_reflected_inertia = 0.0f;

    struct SuspensionState {
        bool grounded = false;
        Vector3 ground_hit_position;
        Vector3 contact_normal;
        Transform3D wheel_transform;
    } suspension_state;

    struct WheelState {
        float angular_velocity = 0.0f;
        float angle = 0.0f;
        float slip_ratio = 0.0f;
        float slip_angle = 0.0f;
        float differential_tan_slip_angle = 0.0f;
        float longitudinal_torque = 0.0f;
        float tire_force_longitudinal = 0.0f;
        float tire_force_lateral = 0.0f;
        float wheel_inertia = 0.0f; // Inertia of the wheel all by itself
        float net_reaction_torque = 0.0f;
    } wheel_state;

    std::optional<Transform3D> suspension_trf_vehicle_local;

    LNVehicleSuspensionState *suspension_solver_state = nullptr;

    void _process_wheel_grounded(const LNVehicleSuspensionSettings::SuspensionSolveResult &p_suspension_result, LNVehicle *p_vehicle_node, LNVehicleWheel *p_wheel_node, const VehicleInputState &p_input_state, float p_delta);
    void _process_wheel_airborne(LNVehicle *p_vehicle_node, LNVehicleWheel *p_wheel_node, const VehicleInputState &p_input_state, float p_delta);
    Vector3 wheel_get_world_forward(LNVehicleWheel *p_wheel_node, LNVehicle *p_vehicle, const VehicleInputState &p_input_state) const;

    Vector3 wheel_get_world_right(LNVehicleWheel *p_wheel_node, LNVehicle *p_vehicle, const VehicleInputState &p_input_state) const;
    void calculate_transient_slip(Ref<LNVehicleWheelSettings> p_wheel_settings, Vector2 p_wheel_velocity, double p_delta);

public:
    virtual String get_debugger_display_name() const override;
    virtual bool has_input() const override;
    virtual int get_output_count() const override;
    virtual void apply_downstream(const DownstreamData &p_data) override;
    virtual void wheel_pre_update(float p_delta, const VehicleInputState &p_input_state, LNVehicle *p_vehicle, LNVehicleWheel *p_wheel_node);
    virtual void wheel_post_update(float p_delta, const VehicleInputState &p_input_state, LNVehicle *p_vehicle, LNVehicleWheel *p_wheel_node);
    virtual void update(float p_delta, const VehicleInputState &p_input_state) override;
    virtual String get_debug_text() const override;
    void apply_arb(LNVehicle *p_vehicle, Ref<LNVehicleWheelShaft> p_other_wheel, float p_arb_stiffness);
    static void _bind_methods() {}
    virtual UpstreamData get_upstream_data() override;
};