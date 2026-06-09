#include "vehicle_suspension.h"
#include "vehicle_suspension_settings.h"

namespace LNVehicleSuspension {

LNVehicleSuspension::SuspensionForceResult compute_suspension_force(Ref<LNVehicleSuspensionSettings> p_suspension_settings, const bool p_is_grounded, const float p_rest_spring_length, const float p_prev_length, const float p_spring_length, const float p_delta) {
    // Compression is defined relative to a "zero compression" reference length:
    //  - If NewLength is shorter than CompressionZeroLength => compressed => positive value
    //  - If NewLength is longer than CompressionZeroLength => overshot => negative value
    const float compression = (p_rest_spring_length + p_suspension_settings->get_rod_length_offset()) - p_spring_length;
    // Normalized compression (dimensionless). We normalize by bump travel as a practical scale.
    // This ratio is intentionally not clamped to [0..1] to preserve diagnostic information.
    //const float compression_ratio = compression / MAX(1e-3f, Config.Common.BumpTravel);

    // Positive velocity means extension (rebound), negative means compression (bump).
    float suspension_velocity = (p_spring_length - p_prev_length) / p_delta;

    // Spring force uses preload and stiffness. Preload can represent helper spring preload,
    // corner weight biasing, or simply to avoid zero-force at rest in some setups.
    const float spring_preload = 0.0f; // Unimplemented, for now
    float spring_force = (compression + spring_preload) * p_suspension_settings->get_spring_rate();

    // We select bump vs rebound damping coefficients based on velocity sign.
    const bool in_rebound = (suspension_velocity > 0.0f);
    const float damp_slow = in_rebound ? p_suspension_settings->get_rebound_damp_rate()     : p_suspension_settings->get_bump_damp_rate();
    const float damp_fast = in_rebound ? p_suspension_settings->get_rebound_fast_damp_rate() : p_suspension_settings->get_bump_fast_damp_rate();
    const float damp_fast_threshold = in_rebound ? p_suspension_settings->get_rebound_fast_damp_rate_threshold() : p_suspension_settings->get_bump_fast_damp_threshold();

    const float susp_velocity_abs  = Math::abs(suspension_velocity);
    const float susp_vel_sign = (suspension_velocity >= 0.0f) ? 1.0f : -1.0f;
    float damping_force = 0.0f;

    // If under threshold, fall back to the slow damping regime.
    if (susp_velocity_abs <= damp_fast_threshold)
    {
        // low-speed: F = -c * v
        damping_force = -suspension_velocity * damp_slow;
    }
    else
    {
        // High-speed regime: keep force continuous at the knee speed.
        // |F| = Cslow*Vknee + Cfast*(|v| - Vknee)
        const float Fmag = (damp_slow * damp_fast_threshold) + (damp_fast * (susp_velocity_abs - damp_fast_threshold));
        damping_force = -susp_vel_sign * Fmag;
    }

    // Total force.
    float raw_total_force = spring_force + damping_force;
    // Clamp to avoid negative loads (wheel "pulling" the chassis) and to cap spikes.
    const float max_force = 10000000.0f;
    float clamped_total_force = CLAMP(raw_total_force, -max_force, max_force);

    // Flag clamping for diagnostics.
    if (!Math::is_equal_approx(raw_total_force, clamped_total_force))
    {
        print_verbose("Force clamped!");
    }

    return {
        .compression = compression,
        .velocity = suspension_velocity,
        .spring_force = spring_force,
        .damping_force = damping_force,
        .total_force = raw_total_force,
        .clamped_total_force = clamped_total_force
    };
}

WheelFrameOut build_wheel_frame(float p_design_camber, float p_design_toe, const Transform3D p_vehicle_transform, float p_side_sign, float p_static_camber, float p_toe, Vector3 steering_axis_world, float p_steer_angle) {
    const Vector3 base_steering_axis_world = steering_axis_world.normalized();
    const Vector3 base_steering_axis_right_world = p_vehicle_transform.basis.xform(Vector3(0.0, 0.0, -1.0f)).cross(base_steering_axis_world).normalized();
    const Vector3 base_steering_axis_forward_world = base_steering_axis_right_world.cross(base_steering_axis_world).normalized();

    const float camber = (p_side_sign) * (p_static_camber - p_design_camber);

    Plane forward_plane = Plane(Vector3(base_steering_axis_forward_world));
    
    const Vector3 steering_axis_up_after_sai_world = base_steering_axis_world.rotated(base_steering_axis_forward_world, camber).normalized();
    const Vector3 steering_axis_right_after_sai_world = base_steering_axis_right_world.rotated(base_steering_axis_forward_world, camber).normalized();

    const float toe = -p_steer_angle + (p_side_sign) * p_design_toe;

    const Vector3 up = steering_axis_up_after_sai_world.normalized();
    const Vector3 right = steering_axis_right_after_sai_world.rotated(steering_axis_up_after_sai_world, toe).normalized();
    const Vector3 forward = base_steering_axis_forward_world.rotated(steering_axis_up_after_sai_world, toe).normalized();

    return {
        .steering_axis_up_world = up,
        .steering_axis_right_world = right,
        .steering_axis_forward_world = forward
    };
}

WheelIntersectionResult do_wheel_intersection(const Node3D *p_vehicle, Vector3 p_wheel_right, Vector3 p_wheel_up, float p_wheel_width, float p_wheel_radius, Vector3 p_wheel_center, float p_muf_width, float p_muf_long) {
    Vector3 right_offset = p_wheel_right * Math::lerp(-p_wheel_width * 0.5f, p_wheel_width * 0.5f, p_muf_width);
    right_offset = Vector3();
    const float SWEEP_RADS = Math::deg_to_rad(30.0f) * 0.5f;
    Vector3 vertical_offset = (-p_wheel_up  * p_wheel_radius).rotated(p_wheel_right, Math::lerp(-SWEEP_RADS, SWEEP_RADS, p_muf_long));
    Vector3 from = p_wheel_center + right_offset;
    Vector3 to = from + vertical_offset;


    PhysicsDirectSpaceState3D *dss = p_vehicle->get_world_3d()->get_direct_space_state();
    Ref<PhysicsRayQueryParameters3D> params;
    params.instantiate();
    params->set_from(from);
    params->set_to(to);
    
    Color wheel_color = Color("BLUE");
    wheel_color.a = 0.15f;
    DebugOverlay::circle_with_dir(p_wheel_center, p_wheel_right, p_wheel_radius, wheel_color);

    if (Dictionary hit_result = dss->intersect_ray(params); !hit_result.is_empty()) {
        Vector3 intersection = hit_result["position"];
        intersection -= right_offset;
        intersection -= vertical_offset;

        return WheelIntersectionResult {
            .hit = true,
            .wheel_center_position = intersection,
            .ground_hit_position = hit_result["position"],
            .ground_normal = hit_result["normal"]
        };
    }

    return {
        .hit = false
    };
}

}