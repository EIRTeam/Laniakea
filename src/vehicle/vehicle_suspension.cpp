#include "vehicle_suspension.h"
#include "debug/debug_overlay.h"
#include "math.h"
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
	const float damp_slow = in_rebound ? p_suspension_settings->get_rebound_damp_rate() : p_suspension_settings->get_bump_damp_rate();
	const float damp_fast = in_rebound ? p_suspension_settings->get_rebound_fast_damp_rate() : p_suspension_settings->get_bump_fast_damp_rate();
	const float damp_fast_threshold = in_rebound ? p_suspension_settings->get_rebound_fast_damp_rate_threshold() : p_suspension_settings->get_bump_fast_damp_threshold();

	const float susp_velocity_abs = Math::abs(suspension_velocity);
	const float susp_vel_sign = (suspension_velocity >= 0.0f) ? 1.0f : -1.0f;
	float damping_force = 0.0f;

	// If under threshold, fall back to the slow damping regime.
	if (susp_velocity_abs <= damp_fast_threshold) {
		// low-speed: F = -c * v
		damping_force = -suspension_velocity * damp_slow;
	} else {
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
	if (!Math::is_equal_approx(raw_total_force, clamped_total_force)) {
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

WheelFrameOut build_wheel_frame(const WheelFrameComputationParams &p_params) {
	const Vector3 base_steering_axis_world = p_params.steering_axis_world.normalized();
	const Vector3 base_steering_axis_right_world = p_params.vehicle_transform.basis.xform(Vector3(0.0, 0.0, -1.0f)).cross(base_steering_axis_world).normalized();
	const Vector3 base_steering_axis_forward_world = base_steering_axis_right_world.cross(base_steering_axis_world).normalized();
	float steering_angle_signed = 0.0f;

	if (p_params.use_steering) {
		Transform3D wheel_trf = Transform3D();
		wheel_trf.basis = Basis(base_steering_axis_right_world * p_params.side_sign, base_steering_axis_world, base_steering_axis_forward_world);
		wheel_trf.origin = p_params.bottom_ball_joint_world;
		const Vector3 wheel_tie_rod_base_position_world = wheel_trf.xform(p_params.tie_rod_wheel_local);
		const Vector3 tie_rotation_center = p_params.bottom_ball_joint_world + (wheel_tie_rod_base_position_world - p_params.bottom_ball_joint_world).project(base_steering_axis_world);

		const Vector2 steering_rack_balljoint_2d = LNMath::to_plane_2d(p_params.tie_rod_rack_position_world, tie_rotation_center, base_steering_axis_world);
		const Vector2 tie_rod_base_position_2d = LNMath::to_plane_2d(wheel_tie_rod_base_position_world, tie_rotation_center, base_steering_axis_world);
		const Vector2 tie_rod_steer_center_2d = LNMath::to_plane_2d(tie_rotation_center, tie_rotation_center, base_steering_axis_world);

		const float tie_rod_hub_radius = tie_rod_steer_center_2d.distance_to(tie_rod_base_position_2d);

		const float axial_offset = p_params.tie_rod_rack_position_world.distance_to(LNMath::from_plane_2d(steering_rack_balljoint_2d, tie_rotation_center, base_steering_axis_world));

		const float tie_rod_length_project_sq = p_params.tie_rod_length * p_params.tie_rod_length - axial_offset * axial_offset;
		const float tie_rod_length_project = Math::sqrt(tie_rod_length_project_sq);

		Vector2 outputs[2];

		const int output_count = LNMath::circle_intersect_2d(steering_rack_balljoint_2d, tie_rod_length_project, tie_rod_steer_center_2d, tie_rod_hub_radius, outputs[0], outputs[1]);

		Vector3 tie_rod_wheel_position_world;

		if (output_count == 2) {
			const Vector3 a = LNMath::from_plane_2d(outputs[0], tie_rotation_center, base_steering_axis_world);
			const Vector3 b = LNMath::from_plane_2d(outputs[1], tie_rotation_center, base_steering_axis_world);

			const float dist_a = a.distance_squared_to(wheel_tie_rod_base_position_world);
			const float dist_b = b.distance_squared_to(wheel_tie_rod_base_position_world);
			tie_rod_wheel_position_world = dist_a < dist_b ? a : b;
		} else if (output_count == 1) {
			tie_rod_wheel_position_world = LNMath::from_plane_2d(outputs[0], tie_rotation_center, base_steering_axis_world);
		}

		DEV_ASSERT(output_count > 0);

		steering_angle_signed = -(tie_rod_wheel_position_world - tie_rotation_center).signed_angle_to(wheel_tie_rod_base_position_world - tie_rotation_center, base_steering_axis_world);

		DebugOverlay::line(tie_rod_wheel_position_world, p_params.tie_rod_rack_position_world, Color("Black"));
	}

	const float camber = (p_params.side_sign) * (p_params.setup_camber - p_params.design_zero_camber);

	const Vector3 steering_axis_up_after_sai_world = base_steering_axis_world.rotated(base_steering_axis_forward_world, camber).normalized();
	const Vector3 steering_axis_right_after_sai_world = base_steering_axis_right_world.rotated(base_steering_axis_forward_world, camber).normalized();

	const float toe = steering_angle_signed + (p_params.side_sign) * p_params.design_zero_toe;

	const Vector3 up = steering_axis_up_after_sai_world.rotated(base_steering_axis_world, toe).normalized();
	const Vector3 right = steering_axis_right_after_sai_world.rotated(base_steering_axis_world, toe).normalized();
	const Vector3 forward = base_steering_axis_forward_world.rotated(base_steering_axis_world, toe).normalized();

	return {
		.steering_axis_up_world = up,
		.steering_axis_right_world = right,
		.steering_axis_forward_world = forward,
		.steering_rod_hub = Basis(right, up, forward).xform(p_params.tie_rod_wheel_local)
	};
}

WheelIntersectionResult do_wheel_intersection(const Node3D *p_vehicle, Vector3 p_wheel_right, Vector3 p_wheel_up, float p_wheel_width, float p_wheel_radius, Vector3 p_wheel_center, float p_muf_width, float p_muf_long) {
	Vector3 right_offset = p_wheel_right * Math::lerp(-p_wheel_width * 0.5f, p_wheel_width * 0.5f, p_muf_width);
	right_offset = Vector3();
	const float SWEEP_RADS = Math::deg_to_rad(30.0f) * 0.5f;
	Vector3 vertical_offset = (-p_wheel_up * p_wheel_radius).rotated(p_wheel_right, Math::lerp(-SWEEP_RADS, SWEEP_RADS, p_muf_long));
	Vector3 from = p_wheel_center + right_offset;
	Vector3 to = from + vertical_offset;

	PhysicsDirectSpaceState3D *dss = p_vehicle->get_world_3d()->get_direct_space_state();
	Ref<PhysicsRayQueryParameters3D> params;
	params.instantiate();
	params->set_from(from);
	params->set_to(to);

	Color wheel_color = Color("BLUE");
	wheel_color.a = 0.15f;
	//DebugOverlay::circle_with_dir(p_wheel_center, p_wheel_right, p_wheel_radius, wheel_color);

	if (Dictionary hit_result = dss->intersect_ray(params); !hit_result.is_empty()) {
		Vector3 intersection = hit_result["position"];
		intersection -= right_offset;
		intersection -= vertical_offset;

		return WheelIntersectionResult{
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

} //namespace LNVehicleSuspension
