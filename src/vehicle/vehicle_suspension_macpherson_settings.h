#pragma once

#include "bind_macros.h"
#include "vehicle/vehicle.h"
#include "vehicle/vehicle_suspension.h"
#include "vehicle/vehicle_suspension_settings.h"
#include "vehicle/vehicle_suspension_state.h"
#include "vehicle/vehicle_wheel_settings.h"
#include "debug/debug_overlay.h"
#include "math.h"

class LNVehicleMacPhersonSuspensionSettings : public LNVehicleSuspensionSettings {
    GDCLASS(LNVehicleMacPhersonSuspensionSettings, LNVehicleSuspensionSettings);
public:
    struct LNVehicleMacPhersonSuspensionState : public LNVehicleSuspensionState {
        Vector3 bottom_tyre_ball_joint_suspension_local;
    };
private:
    WishboneSettings bottom_wishbone;
    struct MacPhersonCache {
        float design_camber;
        float design_toe;
        Vector3 bottom_balljoint_hub_local;
        Vector3 hub_wheel_local;

        // These will be in suspension space
        // pivot center, which is the outer ball joint projected on the line formed by the
        // inner wishbone attachment points
        Vector3 pivot_center;
        // Pivot center axis to form a 2D plane
        // these are in vehicle space
        Vector3 pivot_axis_right;
        Vector3 pivot_axis_up;
        float bottom_control_arm_length_2d = 0.0f;

        float strut_extension_min = 0.0f;
        float strut_extension_max = 0.0f;
    };

    std::optional<MacPhersonCache> macpherson_cache;
public:
    MAKE_SETTER_GETTER_VALUE(Vector3, bottom_wishbone_front, bottom_wishbone.front);
    MAKE_SETTER_GETTER_VALUE(Vector3, bottom_wishbone_rear, bottom_wishbone.rear);
    MAKE_SETTER_GETTER_VALUE(Vector3, bottom_wishbone_tyre, bottom_wishbone.tyre);

    virtual LNVehicleSuspensionState* create_suspension_state() const override;

    static void _bind_methods();
    float get_rest_spring_length() const;

    void _rebuild_cache();
    bool solve_ball_joint_location(const Vector3 &p_upper_shock_mount_world, float p_side_sign, const float &p_control_arm_length, const float &p_strut_length, const Vector3 &p_pivot_axis_proj, const Vector3 &p_pivot_axis_right, const Vector3 &p_pivot_axis_up, Vector3 &p_r_out_intersection);
    SuspensionSolveResult solve(Ref<LNVehicleWheelSettings> p_wheel_settings, LNVehicleWheelPosition p_wheel_position, const Node3D *p_vehicle, double p_delta, LNVehicleSuspensionState *p_state) override;
};