#include "vehicle_suspension_macpherson_settings.h"

#include "bind_macros.h"
#include "godot_cpp/classes/geometry2d.hpp"
#include "math.h"
#include "vehicle/vehicle_suspension.h"
#include "vehicle/vehicle_suspension_settings.h"
#include "vehicle/vehicle_suspension_state.h"

LNVehicleSuspensionState *LNVehicleMacPhersonSuspensionSettings::create_suspension_state() const {
	LNVehicleMacPhersonSuspensionState *state = memnew(LNVehicleMacPhersonSuspensionState);
	state->shock_length = get_strut_car().distance_to(get_bottom_wishbone_tyre());
	state->bottom_tyre_ball_joint_suspension_local = get_bottom_wishbone_tyre();
	state->prev_shock_length = state->shock_length;
	return state;
}

void LNVehicleMacPhersonSuspensionSettings::_bind_methods() {
	MAKE_BIND_VECTOR3(LNVehicleMacPhersonSuspensionSettings, bottom_wishbone_front);
	MAKE_BIND_VECTOR3(LNVehicleMacPhersonSuspensionSettings, bottom_wishbone_rear);
	MAKE_BIND_VECTOR3(LNVehicleMacPhersonSuspensionSettings, bottom_wishbone_tyre);
	MAKE_BIND_VECTOR3(LNVehicleMacPhersonSuspensionSettings, steering_rod_hub);
	MAKE_BIND_VECTOR3(LNVehicleMacPhersonSuspensionSettings, steering_rod_rack);
}

float LNVehicleMacPhersonSuspensionSettings::get_rest_spring_length() const {
	return get_bottom_wishbone_tyre().distance_to(get_strut_car()) + get_rod_length_offset();
}

void LNVehicleMacPhersonSuspensionSettings::_rebuild_cache() {
	Transform3D suspension_trf = Transform3D();

	LNVehicleSuspension::WheelFrameOut test_frame = LNVehicleSuspension::build_wheel_frame({ .bottom_ball_joint_world = get_bottom_wishbone_tyre(),
			.steering_axis_world = get_bottom_wishbone_tyre().direction_to(get_strut_car()),
			.use_steering = false });

	Transform3D original_steering_trf = Transform3D();
	original_steering_trf.basis = Basis(test_frame.steering_axis_right_world, test_frame.steering_axis_up_world, test_frame.steering_axis_forward_world);
	original_steering_trf.origin = suspension_trf.xform(get_bottom_wishbone_tyre());

	Transform3D hub_trf = Transform3D();
	hub_trf.basis = Basis(test_frame.steering_axis_right_world, test_frame.steering_axis_up_world, test_frame.steering_axis_forward_world);
	hub_trf.origin = suspension_trf.xform(Vector3());

	const Plane camber_plane = Plane(Vector3(0, 0, 1)); // world forward as normal
	Vector3 axis_camber_plane = camber_plane.project(test_frame.steering_axis_up_world).normalized();
	Vector3 up_camber_plane = Vector3(0, 1, 0); // world Y needs no projection, it's already in the plane

	const float design_camber = up_camber_plane.signed_angle_to(
			axis_camber_plane,
			Vector3(0, 0, 1));

	const Plane world_horizontal = Plane(Vector3(0, 1, 0));
	Vector3 wheel_fwd_flat = world_horizontal.project(test_frame.steering_axis_forward_world).normalized();
	Vector3 world_fwd_flat = Vector3(0, 0, 1); // your car's forward

	// Angle to rotate the steering axis around its own up to zero the toe
	const float design_toe = wheel_fwd_flat.signed_angle_to(
			world_fwd_flat,
			test_frame.steering_axis_up_world // rotate around hub up so camber is preserved
	);

	macpherson_cache = MacPhersonCache {
		.design_camber = design_camber,
		.design_toe = design_toe,
		.bottom_balljoint_hub_local = hub_trf.affine_inverse().xform(suspension_trf.xform(get_bottom_wishbone_tyre())),
		.hub_wheel_local = original_steering_trf.affine_inverse().xform(Vector3()),
		.wheel_local_steer = original_steering_trf.affine_inverse().xform(get_steering_rod_hub())
	};

	const Vector3 inner_rear_attachment_suspension_space = get_bottom_wishbone_rear();
	const Vector3 inner_front_attachment_suspension_space = get_bottom_wishbone_front();
	const Vector3 balljoint_suspension_space = get_bottom_wishbone_tyre();

	const Vector3 inner_rear_to_outer = (balljoint_suspension_space - inner_rear_attachment_suspension_space);
	const Vector3 inner_rear_to_inner_front = (inner_front_attachment_suspension_space - inner_rear_attachment_suspension_space);
	const Vector3 control_arm_proj = inner_rear_attachment_suspension_space + inner_rear_to_outer.project(inner_rear_to_inner_front);
	const Vector3 control_arm_proj_right = balljoint_suspension_space.direction_to(control_arm_proj);
	const Vector3 control_arm_proj_up = control_arm_proj_right.cross(inner_rear_to_inner_front.normalized());

	macpherson_cache->pivot_center = control_arm_proj;
	// These define the plane on which the ball joint moves
	macpherson_cache->pivot_axis_right = control_arm_proj_right;
	macpherson_cache->pivot_axis_up = control_arm_proj_up;

	// Essentially, we project the ball joint position on the axis defined by the front and rear wishbone attachment points
	// this puts it on a plane parallel to world forward, which lets us calculate the control arm length in that plane
	// this is later used for solving the ball joint position
	macpherson_cache->bottom_control_arm_length_2d = control_arm_proj.distance_to(balljoint_suspension_space);

	// Find minimum and maximum strut length, until it becomes degenerate
	// do note this isn't perfect yet, the steering isn't taken into account at all
	float design_0_length = get_strut_car().distance_to(get_bottom_wishbone_tyre());

	float maximum_test = get_strut_car().distance_to(control_arm_proj) + macpherson_cache->bottom_control_arm_length_2d;
	float minimum_test = maximum_test - macpherson_cache->bottom_control_arm_length_2d * 2.0f;

	const float total_vertical_travel = maximum_test - minimum_test;

	const float step_granularity = 0.002f;

	int steps = Math::ceil(total_vertical_travel / step_granularity);

	std::optional<float> minimum_shock;
	std::optional<float> maximum_shock;

	for (int i = 0; i < steps; i++) {
		const float extension = Math::lerp(minimum_test, maximum_test, (i + 1) / static_cast<float>(steps));
		Vector3 intersection;
		if (solve_ball_joint_location(get_strut_car(), 1.0f, macpherson_cache->bottom_control_arm_length_2d, extension, control_arm_proj, control_arm_proj_right, control_arm_proj_up, intersection)) {
			if (!minimum_shock.has_value()) {
				minimum_shock = extension;
			}

			maximum_shock = extension;
		} else if (maximum_shock.has_value()) {
			break;
		} else {
		}
	}

	DEV_ASSERT(minimum_shock.has_value());
	DEV_ASSERT(maximum_shock.has_value());

	macpherson_cache->strut_extension_min = *minimum_shock;
	macpherson_cache->strut_extension_max = *maximum_shock;
}

bool LNVehicleMacPhersonSuspensionSettings::solve_ball_joint_location(const Vector3 &p_upper_shock_mount_world, float p_side_sign, const float &p_control_arm_length, const float &p_strut_length, const Vector3 &p_pivot_axis_proj, const Vector3 &p_pivot_axis_right, const Vector3 &p_pivot_axis_up, Vector3 &p_r_out_intersection) {
	// ---- Planar 2D solve ----------------------------------------------------
	// We solve in a 2D plane defined by (suspension_right_world, suspension_up_world),
	// with the component origin as the 2D origin.
	//
	// Interpretation:
	//  - Circle A: centered at component origin, radius = control arm length
	//  - Circle B: centered at upper shock mount projected into the plane, radius = strut length
	//
	// Their intersection(s) define possible ball joint locations in the mechanism plane.
	const Vector2 suspension_origin_2d = Vector2();
	const Vector2 upper_shock_mount_2d = LNMath::to_plane_2d(p_upper_shock_mount_world,
			p_pivot_axis_proj, p_pivot_axis_right, p_pivot_axis_up);

	Vector2 intersection_a_2d;
	Vector2 intersection_b_2d;
	const int32_t intersection_count = LNMath::circle_intersect(suspension_origin_2d,
			p_control_arm_length,
			upper_shock_mount_2d,
			p_strut_length,
			intersection_a_2d,
			intersection_b_2d,
			1e-4f);

	if (intersection_count == 2) {
		// Two possible solutions: choose the "outboard" one.
		// We define outboard as the intersection with the larger signed X when mirrored by side.
		// SideSign ensures correct selection for left/right.
		const float outboard_1 = intersection_a_2d.x * (p_side_sign);
		const float outboard_2 = intersection_b_2d.x * (p_side_sign);

		const Vector2 chosen_2d = (outboard_1 <= outboard_2) ? intersection_a_2d : intersection_b_2d;

		p_r_out_intersection = LNMath::from_plane_2d(chosen_2d,
				p_pivot_axis_proj, p_pivot_axis_right, p_pivot_axis_up);
		return true;
	}
	if (intersection_count == 1) {
		// Tangent case: a single intersection (mechanism at boundary of feasibility).
		p_r_out_intersection = LNMath::from_plane_2d(intersection_a_2d,
				p_pivot_axis_proj, p_pivot_axis_right, p_pivot_axis_up);
		return true;
	}

	// No solution (circles do not intersect).
	p_r_out_intersection = Vector3();
	return false;
}

LNVehicleSuspensionSettings::SuspensionSolveResult LNVehicleMacPhersonSuspensionSettings::solve(Ref<LNVehicleWheelSettings> p_wheel_settings, Vector3 p_tie_rod_rack_position_world, LNVehicleWheelPosition p_wheel_position, const Node3D *p_vehicle, double p_delta, LNVehicleSuspensionState *p_state) {
	LNVehicleMacPhersonSuspensionState *state = dynamic_cast<LNVehicleMacPhersonSuspensionState *>(p_state);

	if (!macpherson_cache.has_value()) {
		_rebuild_cache();
	}

	DEV_ASSERT(state != nullptr);

	const float side_sign = p_wheel_position == WHEEL_FL || p_wheel_position == WHEEL_RL ? 1.0 : -1.0f;

	const Transform3D vehicle_transform = p_vehicle->get_global_transform();

	const Vector3 bottom_tyre_ball_joint_world = state->suspension_transform_world.xform(state->bottom_tyre_ball_joint_suspension_local);
	const Vector3 strut_attachment_world = state->suspension_transform_world.xform(get_strut_car());

	LNVehicleSuspension::WheelFrameOut wheel_frame = LNVehicleSuspension::build_wheel_frame({ .tie_rod_rack_position_world = p_tie_rod_rack_position_world,
			.tie_rod_wheel_local = macpherson_cache->wheel_local_steer,
			.tie_rod_length = get_steering_rod_hub().distance_to(get_steering_rod_rack()),
			.bottom_ball_joint_world = bottom_tyre_ball_joint_world,
			.design_zero_camber = macpherson_cache->design_camber,
			.design_zero_toe = macpherson_cache->design_toe,

			.vehicle_transform = p_vehicle->get_global_transform(),
			.side_sign = side_sign,
			.setup_camber = Math::deg_to_rad(get_static_camber_degrees()),
			.steering_axis_world = bottom_tyre_ball_joint_world.direction_to(strut_attachment_world),
			.use_steering = state->can_steer });

	const float tire_width = p_wheel_settings->get_width();
	const float tire_radius = p_wheel_settings->get_radius();

	const Vector3 inner_rear_attachment_world = state->suspension_transform_world.xform(get_bottom_wishbone_rear());
	const Vector3 inner_front_attachment_world = state->suspension_transform_world.xform(get_bottom_wishbone_front());

	const Vector3 control_arm_proj = p_state->suspension_transform_world.xform(macpherson_cache->pivot_center);
	const Vector3 control_arm_proj_right = p_state->suspension_transform_world.basis.xform(macpherson_cache->pivot_axis_right);
	const Vector3 control_arm_proj_up = p_state->suspension_transform_world.basis.xform(macpherson_cache->pivot_axis_up);

	const float control_arm_length_2d = macpherson_cache->bottom_control_arm_length_2d;

	float strut_length = p_state->shock_length;

	std::optional<LNVehicleSuspension::WheelIntersectionResult> highest_hit_world;

	const int cast_count_width = 3;
	const int cast_count_radial = 6;

	// Wheel -> ground casting
	{
		Transform3D wheel_trf = Transform3D(
				Basis(
						side_sign * wheel_frame.steering_axis_right_world,
						wheel_frame.steering_axis_up_world,
						wheel_frame.steering_axis_forward_world),
				bottom_tyre_ball_joint_world);

		Vector3 hub_pos_world = wheel_trf.xform(macpherson_cache->hub_wheel_local);

		for (int i = 0; i < cast_count_width; i++) {
			const float muf_long = i / static_cast<float>(cast_count_width - 1);
			for (int j = 0; j < cast_count_radial; j++) {
				const float muf_radial = j / static_cast<float>(cast_count_radial - 1);
				LNVehicleSuspension::WheelIntersectionResult intersection_result = LNVehicleSuspension::do_wheel_intersection(p_vehicle, (-side_sign) * wheel_frame.steering_axis_right_world, wheel_frame.steering_axis_up_world, tire_width, tire_radius, hub_pos_world, muf_long, muf_radial);
				if (!intersection_result.hit) {
					continue;
				}

				if (!highest_hit_world.has_value()) {
					highest_hit_world = intersection_result;
					continue;
				}

				if (p_vehicle->to_local(intersection_result.wheel_center_position).y > p_vehicle->to_local(highest_hit_world->wheel_center_position).y) {
					highest_hit_world = intersection_result;
				}
			}
		}
	}

	if (highest_hit_world.has_value()) {
		// We are on the ground, find an estimated ball joint position to calculate the new strut length
		Transform3D hub_trf;
		hub_trf.basis = Basis(
				side_sign * wheel_frame.steering_axis_right_world,
				wheel_frame.steering_axis_up_world,
				wheel_frame.steering_axis_forward_world);
		hub_trf.origin = highest_hit_world->wheel_center_position;
		Vector3 new_balljoint_pos = hub_trf.xform(macpherson_cache->bottom_balljoint_hub_local);
		strut_length = strut_attachment_world.distance_to(new_balljoint_pos);
		strut_length = CLAMP(strut_length, macpherson_cache->strut_extension_min, macpherson_cache->strut_extension_max);
	}

	// Spring forces
	LNVehicleSuspension::SuspensionForceResult force_result;

	if (highest_hit_world.has_value()) {
		state->prev_shock_length = state->shock_length;
		force_result = LNVehicleSuspension::compute_suspension_force(this, true, get_rest_spring_length(), state->prev_shock_length, strut_length, p_delta);
		// Spring force went negative, meaning we have to lift the wheels off the ground
		if (force_result.spring_force < 0.0f) {
			highest_hit_world.reset();
		}
	} else if (!highest_hit_world.has_value()) {
		// wheel is in the air, extend it
		// spring length has not changed in this frame since we didn't get a hit, so we will use state->prev_strut_length to estimate the velocity
		// TODO: Make this affect the chassis
		force_result = LNVehicleSuspension::compute_suspension_force(this, false, get_rest_spring_length(), state->prev_shock_length, p_state->shock_length, p_delta);
	}

	if (!highest_hit_world.has_value()) {
		// Airborne case, we need to make the spring get affected by gravity
		float gravity_force = p_wheel_settings->get_mass() * 9.81f * bottom_tyre_ball_joint_world.direction_to(strut_attachment_world).dot(Vector3(0, -1, 0));
		float net_force = force_result.clamped_total_force - gravity_force;
		float suspension_acceleration = net_force / p_wheel_settings->get_mass();
		float new_strut_length = p_state->shock_length + force_result.velocity * p_delta + 0.5f * suspension_acceleration * p_delta * p_delta;
		strut_length = CLAMP(new_strut_length, macpherson_cache->strut_extension_min, macpherson_cache->strut_extension_max);
		state->prev_shock_length = state->shock_length;
		state->shock_length = strut_length;
	}

	// Solve ball joint location

	Vector3 strut_attachment_suspension_space = get_strut_car();

	if (solve_ball_joint_location(strut_attachment_suspension_space, 1.0f, control_arm_length_2d, strut_length, macpherson_cache->pivot_center, macpherson_cache->pivot_axis_right, macpherson_cache->pivot_axis_up, state->bottom_tyre_ball_joint_suspension_local)) {
		float prev_shock_length = state->shock_length;

		Transform3D new_steering_trf = Transform3D(
				Basis(
						wheel_frame.steering_axis_right_world,
						wheel_frame.steering_axis_up_world,
						wheel_frame.steering_axis_forward_world),
				p_state->suspension_transform_world.xform(state->bottom_tyre_ball_joint_suspension_local));

		const Vector3 hub_pos_world = new_steering_trf.xform(macpherson_cache->hub_wheel_local * Vector3(side_sign, 1.0f, 1.0f));

		state->shock_length = strut_length;

		const Vector3 wishbone_front = state->suspension_transform_world.xform(get_bottom_wishbone_front());
		const Vector3 wishbone_rear = state->suspension_transform_world.xform(get_bottom_wishbone_rear());

		DebugOverlay::filled_arrow(wishbone_front, state->suspension_transform_world.xform(state->bottom_tyre_ball_joint_suspension_local), 0.05f, Color("BLUE"));
		DebugOverlay::filled_arrow(wishbone_rear, state->suspension_transform_world.xform(state->bottom_tyre_ball_joint_suspension_local), 0.05f, Color("RED"));

		const Plane side_view_plane = Plane(new_steering_trf.basis.get_column(0), new_steering_trf.origin);
		const Plane front_view_plane = Plane(new_steering_trf.basis.get_column(2), new_steering_trf.origin);

		Vector3 n_plane_normal;

		// We need to calculate the n plane, the n plane only applies if we are grounded, as it's used for applying tyre forces.
		if (highest_hit_world.has_value()) {
			// try to find the front view IC
			n_plane_normal = highest_hit_world->ground_normal;
			const Vector3 ray_dir = inner_front_attachment_world.direction_to(inner_rear_attachment_world);
			Vector3 front_view_pivot;
			std::optional<Vector3> front_view_ic;
			std::optional<Vector3> side_view_ic;
			if (front_view_plane.intersects_ray(inner_front_attachment_world, ray_dir, &front_view_pivot)) {
				Vector2 front_view_ball_joint_2d = LNMath::to_plane_2d(new_steering_trf.origin, front_view_plane.get_center(), front_view_plane.get_normal());
				Vector2 front_view_pivot_2d = LNMath::to_plane_2d(front_view_pivot, front_view_plane.get_center(), front_view_plane.get_normal());
				Vector2 upper_attachemnt_2d = LNMath::to_plane_2d(strut_attachment_world, front_view_plane.get_center(), front_view_plane.get_normal());

				Vector2 line_a_2d = front_view_pivot_2d.direction_to(front_view_ball_joint_2d);
				Vector2 line_b_2d = upper_attachemnt_2d.direction_to(front_view_ball_joint_2d);

				SWAP(line_b_2d.x, line_b_2d.y);
				line_b_2d.x = -line_b_2d.x;

				Variant out = Geometry2D::get_singleton()->line_intersects_line(front_view_pivot_2d, line_a_2d, upper_attachemnt_2d, line_b_2d);

				if (out.get_type() != Variant::Type::NIL) {
					const Vector3 front_ic = LNMath::from_plane_2d(out, front_view_plane.get_center(), front_view_plane.get_normal());
					front_view_ic = front_ic;
				}
			}

			// Find the side IC
			if (front_view_ic.has_value()) {
				const Vector3 side_view_plane_origin = new_steering_trf.origin;
				const Vector3 side_view_plane_normal = new_steering_trf.basis.get_column(0);

				const Vector2 side_view_upper_attachemnt = LNMath::to_plane_2d(strut_attachment_world, side_view_plane_origin, side_view_plane_normal);
				const Vector2 side_view_wishbhone_front = LNMath::to_plane_2d(wishbone_front, side_view_plane_origin, side_view_plane_normal);
				const Vector2 side_view_wishbhone_rear = LNMath::to_plane_2d(wishbone_rear, side_view_plane_origin, side_view_plane_normal);
				const Vector2 side_view_ball_joint = LNMath::to_plane_2d(new_steering_trf.origin, side_view_plane_origin, side_view_plane_normal);

				Vector2 line_a = side_view_upper_attachemnt.direction_to(side_view_ball_joint);
				SWAP(line_a.x, line_a.y);
				line_a.x = -line_a.x;

				Vector2 line_b = side_view_wishbhone_front.direction_to(side_view_wishbhone_rear);

				Variant out = Geometry2D::get_singleton()->line_intersects_line(side_view_upper_attachemnt, line_a, side_view_wishbhone_front, line_b);

				if (out.get_type() != Variant::NIL) {
					const Vector3 out_3d = LNMath::from_plane_2d(out, side_view_plane_origin, side_view_plane_normal);
					side_view_ic = out_3d;
				}
			}

			const Vector3 contact_patch = highest_hit_world->ground_hit_position;

			// Successfully found the side and front ICs, calculate the n plane
			if (side_view_ic.has_value() && front_view_ic.has_value()) {
				Vector3 edge_a = *side_view_ic - contact_patch;
				Vector3 edge_b = *front_view_ic - contact_patch;
				n_plane_normal = edge_a.cross(edge_b).normalized();
			}

			Vector3 shock_axis = new_steering_trf.origin.direction_to(strut_attachment_world);
			// In real life, forces would get applied through
			Vector3 force_to_apply = highest_hit_world->ground_normal * shock_axis.dot(highest_hit_world->ground_normal) * force_result.clamped_total_force;
			return {
				.success = true,
				.grounded = true,
				.force_to_apply = force_to_apply,
				.total_force = force_result.clamped_total_force,
				.force_world_position = strut_attachment_world,
				.wheel_transform = Transform3D(new_steering_trf.basis, hub_pos_world),
				.wheel_axis_x = new_steering_trf.basis.get_column(0),
				.wheel_axis_y = new_steering_trf.basis.get_column(1),
				.wheel_axis_z = new_steering_trf.basis.get_column(2),
				.grounded_normal = highest_hit_world->ground_normal,
				.ground_hit_position = highest_hit_world->ground_hit_position,
				.n_plane_normal = n_plane_normal,
				.spring_displacement = force_result.compression
			};
		}
	}

	return {
		.success = true,
		.grounded = false
	};
}
