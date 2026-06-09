#pragma once

#include "godot_cpp/classes/physics_direct_space_state3d.hpp"
#include "godot_cpp/classes/physics_ray_query_parameters3d.hpp"
#include "godot_cpp/classes/world3d.hpp"
#include "godot_cpp/variant/vector3.hpp"
#include "vehicle/vehicle.h"
#include "debug/debug_overlay.h"

using namespace godot;

namespace LNVehicleSuspension {
    struct SuspensionForceResult {
        float compression = 0.0f;

        // Positive = rebound = length increase
        // Negative = bump = length decrease
        float velocity = 0.0f;

        float spring_force = 0.0f;
        float damping_force = 0.0f;

        float total_force = 0.0f;
        float clamped_total_force = 0.0f;
    };

    SuspensionForceResult compute_suspension_force(Ref<LNVehicleSuspensionSettings> p_suspension_settings, const bool p_is_grounded, const float p_rest_spring_length, const float p_prev_length, const float p_spring_length, const float p_delta);

    struct WheelFrameOut {
        Vector3 steering_axis_up_world;
        Vector3 steering_axis_right_world;
        Vector3 steering_axis_forward_world;
    };

    WheelFrameOut build_wheel_frame(const float p_design_camber, float p_design_toe, const Transform3D p_vehicle_transform, float p_side_sign, float p_camber, float p_toe, Vector3 steering_axis_world, float p_steer_angle);

    struct WheelIntersectionResult {
        bool hit = false;
        Vector3 wheel_center_position;
        Vector3 ground_hit_position;
        Vector3 ground_normal;
    };

    WheelIntersectionResult do_wheel_intersection(const Node3D *p_vehicle, Vector3 p_wheel_right, Vector3 p_wheel_up, float p_wheel_width, float p_wheel_radius, Vector3 p_wheel_center, float p_muf_width, float p_muf_long);
};