#include "vehicle_wheel_shaft.h"

#include "godot_cpp/classes/physics_direct_space_state3d.hpp"
#include "godot_cpp/classes/physics_ray_query_parameters3d.hpp"
#include "godot_cpp/classes/world3d.hpp"
#include "godot_cpp/variant/transform3d.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "physics.h"
#include "vehicle/debug_icons.h"
#include "vehicle/shaft.h"
#include "vehicle/steering_rack.h"
#include "vehicle/vehicle.h"
#include "vehicle/vehicle_suspension.h"
#include "vehicle/vehicle_suspension_macpherson_settings.h"
#include "vehicle/vehicle_suspension_settings.h"
#include "vehicle/vehicle_wheel.h"
#include "vehicle/vehicle_wheel_settings.h"
#include "vehicle/wheel_position.h"

Vector2 brush_gdsim(Vector2 p_slip, float p_contact_patch, float p_coefficient_of_friction, float p_tire_stiffness, float p_y_force) {
	// float con_patch = 0.35f;
	// float mu = 0.85f;
	// float tire_stiffness = 5.5f;
	float con_patch = p_contact_patch;
	float mu = p_coefficient_of_friction;
	float tire_stiffness = p_tire_stiffness;

	// 500000 because this line is multiplied by 0.5, while stiffness values are actually in the millions
	// tire_stiffness is a small value just for convenience
	float stiffness = 500000.0f * tire_stiffness * Math::pow(con_patch, 2.0f);
	float friction = mu * p_y_force;
	// "Brush" tire formula
	float deflect = Math::sqrt(Math::pow(stiffness * p_slip.y, 2.0f) + Math::pow(stiffness * Math::tan(p_slip.x), 2.0f));
	if (deflect == 0.0f || !Math::is_finite(deflect)) {
		return Vector2();
	} else {
		Vector2 vector = Vector2();
		float crit_length = friction * (1 - p_slip.y) * con_patch / (2 * deflect);
		if (crit_length >= con_patch) {
			vector.y = stiffness * -p_slip.y / (1.0f - p_slip.y);
			vector.x = stiffness * Math::tan(p_slip.x) / (1 - p_slip.y);
		} else {
			float brushy = (1.0f - friction * (1 - p_slip.y) / (4.0f * deflect)) / deflect;
			vector.y = friction * stiffness * -p_slip.y * brushy;
			vector.x = friction * stiffness * Math::tan(p_slip.x) * brushy;
		}
		DEV_ASSERT(vector.is_finite());
		return vector;
	}
}

void LNVehicleWheelShaft::_process_wheel_grounded(const LNVehicleSuspensionSettings::SuspensionSolveResult &p_suspension_result, LNVehicle *p_vehicle_node, LNVehicleWheel *p_wheel_node, const VehicleInputState &p_input_state, float p_delta) {
	const Ref<LNVehicleSuspensionSettings> suspension_settings = p_wheel_node->get_suspension_settings();
	const Vector3 contact_velocity = LNPhysics::velocity_at_pos(p_vehicle_node, suspension_state.ground_hit_position);

	const Ref<LNVehicleWheelSettings> wheel_settings = p_wheel_node->get_wheel_settings();

	const Vector3 world_wheel_forward = wheel_get_world_forward(p_wheel_node, p_vehicle_node, p_input_state);
	const Vector3 world_wheel_right = wheel_get_world_right(p_wheel_node, p_vehicle_node, p_input_state);

	const float longitudinal_wheel_speed = contact_velocity.dot(world_wheel_forward);
	const float lateral_wheel_speed = contact_velocity.dot(world_wheel_right);

	LNPhysics::apply_force(p_vehicle_node, p_suspension_result.force_to_apply, p_suspension_result.force_world_position - p_vehicle_node->get_global_position());

	calculate_transient_slip(p_wheel_node->get_wheel_settings(), Vector2(lateral_wheel_speed, longitudinal_wheel_speed), p_delta);

	const float y_load = p_suspension_result.force_to_apply.dot(p_suspension_result.grounded_normal);
	Vector2 tire_forces = brush_gdsim(Vector2(wheel_state.slip_angle, wheel_state.slip_ratio), wheel_settings->get_contact_patch(), wheel_settings->get_coefficient_of_friction(), wheel_settings->get_stiffness(), y_load);
	wheel_state.tire_force_longitudinal = tire_forces.y;
	wheel_state.tire_force_lateral = tire_forces.x;

	// Fz: vertical (ground normal)
	Vector3 Fz_dir = p_suspension_result.grounded_normal;

	// Fx: wheel forward projected onto ground plane
	Vector3 wheel_forward = p_suspension_result.wheel_axis_z; // your forward axis
	Vector3 Fx_dir = -(wheel_forward - wheel_forward.dot(Fz_dir) * Fz_dir).normalized();

	// Fy: lateral, in ground plane
	Vector3 Fy_dir = -Fz_dir.cross(Fx_dir);

	DebugOverlay::line(p_suspension_result.ground_hit_position, p_suspension_result.ground_hit_position + Fx_dir, Color("BLUE"));
	DebugOverlay::line(p_suspension_result.ground_hit_position, p_suspension_result.ground_hit_position + Fz_dir, Color("GREEN"));
	DebugOverlay::line(p_suspension_result.ground_hit_position, p_suspension_result.ground_hit_position + Fy_dir, Color("RED"));

	Vector3 F_tire = -wheel_state.tire_force_longitudinal * Fx_dir + -wheel_state.tire_force_lateral * Fy_dir;
	Vector3 force_n_plane = F_tire - p_suspension_result.n_plane_normal * F_tire.dot(p_suspension_result.n_plane_normal);
	const Vector3 force_pos = suspension_state.ground_hit_position;
	DebugOverlay::filled_arrow(force_pos, force_pos + force_n_plane, 0.1f, Color("RED"));
	DebugOverlay::filled_arrow(p_suspension_result.ground_hit_position, p_suspension_result.ground_hit_position + p_suspension_result.n_plane_normal, 0.1f, Color("YELLOW"));
	LNPhysics::apply_force(p_vehicle_node, force_n_plane, suspension_state.ground_hit_position - p_vehicle_node->get_global_position(), Color("RED"));
}

void LNVehicleWheelShaft::_process_wheel_airborne(LNVehicle *p_vehicle_node, LNVehicleWheel *p_wheel_node, const VehicleInputState &p_input_state, float p_delta) {
}

Vector3 LNVehicleWheelShaft::wheel_get_world_forward(LNVehicleWheel *p_wheel_node, LNVehicle *p_vehicle, const VehicleInputState &p_input_state) const {
	return suspension_state.wheel_transform.basis.xform(Vector3(0.0, 0.0, -1.0));
}

Vector3 LNVehicleWheelShaft::wheel_get_world_right(LNVehicleWheel *p_wheel_node, LNVehicle *p_vehicle, const VehicleInputState &p_input_state) const {
	return suspension_state.wheel_transform.basis.xform(Vector3(1.0, 0.0, 0.0));
}

void LNVehicleWheelShaft::calculate_transient_slip(Ref<LNVehicleWheelSettings> p_wheel_settings, Vector2 p_wheel_velocity, double p_delta) {
	static constexpr float longitudinal_relaxation_length = 0.1f;
	static constexpr float lateral_relaxation_length = 0.3f;
	const float clamping_factor = 1.0f;

	const Vector2 contact_patch_velocity = p_wheel_velocity;
	const float wheel_radius = p_wheel_settings->get_radius();
	Vector2 slip_velocity = Vector2(contact_patch_velocity.x, wheel_state.angular_velocity * wheel_radius - contact_patch_velocity.y);
	const float steady_state_slip_angle = Math::atan(slip_velocity.x / MAX(Math::abs(contact_patch_velocity.y), 1.0f));
	const float slip_angle_delta = (p_delta / lateral_relaxation_length) * (slip_velocity.x - MAX(contact_patch_velocity.y, clamping_factor) * wheel_state.slip_angle);
	wheel_state.differential_tan_slip_angle = CLAMP(wheel_state.differential_tan_slip_angle + slip_angle_delta, -Math::abs(Math::tan(steady_state_slip_angle)), Math::abs(Math::tan(steady_state_slip_angle)));

	const float steady_state_slip_ratio = slip_velocity.y / MAX(Math::abs(contact_patch_velocity.y), clamping_factor);
	float slip_ratio_delta = (p_delta / longitudinal_relaxation_length) * (slip_velocity.y - MAX(Math::abs(contact_patch_velocity.y), clamping_factor) * wheel_state.slip_ratio);
	wheel_state.slip_ratio = CLAMP(wheel_state.slip_ratio + slip_ratio_delta, -Math::abs(steady_state_slip_ratio), Math::abs(steady_state_slip_ratio));
	wheel_state.slip_angle = Math::atan(wheel_state.differential_tan_slip_angle);
}

String LNVehicleWheelShaft::get_debugger_display_name() const {
	return String::utf8(LNDebugIcons::TIRE) + " " + get_name();
}

bool LNVehicleWheelShaft::has_input() const {
	return true;
}

int LNVehicleWheelShaft::get_output_count() const {
	return 0;
}

void LNVehicleWheelShaft::apply_downstream(const DownstreamData &p_data) {
	drive_torque = p_data.torque;
	drive_reflected_inertia = p_data.reflected_inertia;
}

void LNVehicleWheelShaft::wheel_pre_update(float p_delta, const Vector3 &p_rack_steer_rod_attachment_world, const VehicleInputState &p_input_state, LNVehicle *p_vehicle, LNVehicleWheel *p_wheel_node) {
	wheel_state.tire_force_lateral = 0.0f;
	wheel_state.tire_force_longitudinal = 0.0f;

	Ref<LNVehicleSuspensionSettings> suspension_settings = p_wheel_node->get_suspension_settings();
	Ref<LNVehicleWheelSettings> wheel_settings = p_wheel_node->get_wheel_settings();
	if (suspension_settings.is_null() || wheel_settings.is_null()) {
		return;
	}

	if (suspension_solver_state == nullptr) {
		suspension_solver_state = suspension_settings->create_suspension_state();
	}

	const float wheel_sign = p_wheel_node->get_wheel_position() == WHEEL_RL || p_wheel_node->get_wheel_position() == WHEEL_FL ? 1.0f : -1.0f;

	suspension_solver_state->suspension_transform_world = get_suspension_transform(p_vehicle, p_wheel_node);
	suspension_solver_state->steering_angle_rads = p_wheel_node->get_steerable() ? p_input_state.steer * Math::deg_to_rad(35.0f) : 0.0f;
	suspension_solver_state->can_steer = p_wheel_node->get_steerable();

	LNVehicleSuspensionSettings::SuspensionSolveResult solve_result = suspension_settings->solve(wheel_settings, p_rack_steer_rod_attachment_world, p_wheel_node->get_wheel_position(), p_vehicle, p_delta, suspension_solver_state);

	suspension_state.compression = solve_result.spring_displacement;

	if (!solve_result.success) {
		// Something's fucked
		DEV_ASSERT(false);
	}

	suspension_state.grounded = solve_result.grounded;
	suspension_state.wheel_transform = solve_result.wheel_transform;
	if (!solve_result.grounded) {
		// Freewheel
		_process_wheel_airborne(p_vehicle, p_wheel_node, p_input_state, p_delta);
	} else {
		suspension_state.contact_normal = solve_result.grounded_normal;
		suspension_state.ground_hit_position = solve_result.ground_hit_position;
		_process_wheel_grounded(solve_result, p_vehicle, p_wheel_node, p_input_state, p_delta);
	}
	const float wheel_mass = wheel_settings->get_mass();
	const float wheel_radius = p_wheel_node->get_wheel_settings()->get_radius();
	wheel_state.wheel_inertia = (0.5f * wheel_mass * wheel_radius * wheel_radius);

	wheel_state.net_reaction_torque = wheel_state.tire_force_longitudinal * wheel_settings->get_radius();
	//p_wheel_node->set_global_transform(solve_result.wheel_transform);
}

void LNVehicleWheelShaft::wheel_post_update(float p_delta, const VehicleInputState &p_input_state, LNVehicle *p_vehicle, LNVehicleWheel *p_wheel_node) {
	Ref<LNVehicleWheelSettings> wheel_settings = p_wheel_node->get_wheel_settings();
	const float wheel_radius = wheel_settings->get_radius();
	const Vector3 world_wheel_direction = p_vehicle->get_global_basis().xform(Vector3(0.0, -1.0, 0.0));
	Ref<LNVehicleSuspensionSettings> suspension_settings = p_wheel_node->get_suspension_settings();

	const float wheel_mass = wheel_settings->get_mass();

	const float T_ext = drive_torque + wheel_state.tire_force_longitudinal * wheel_radius;

	DEV_ASSERT(Math::is_finite(T_ext));
	DEV_ASSERT(Math::is_finite(wheel_radius));

	const float wheel_moment = drive_reflected_inertia + wheel_state.wheel_inertia;
	const float torque_required_to_stop_wheel = -(T_ext + ((wheel_state.angular_velocity * wheel_moment) / p_delta));
	const float brake_torque_available = 1000.0f * p_input_state.brake;
	float brake_torque = CLAMP(torque_required_to_stop_wheel, -brake_torque_available, brake_torque_available);

	// Integrate wheel using the clamped force
	wheel_state.angular_velocity += ((T_ext + brake_torque) / wheel_moment) * p_delta;
	wheel_state.angle += wheel_state.angular_velocity * p_delta;
	// wheel->set_rotation(Vector3(wheel_data.angle * p_delta, wheel->get_steerable() ? input.steer * Math_PI * -0.25f : 0.0f, 0.0f));
	p_wheel_node->set_rotation(Vector3(-wheel_state.angle, p_wheel_node->get_steerable() ? p_input_state.steer * Math_PI * -0.25f : 0.0f, 0.0f));
}

void LNVehicleWheelShaft::update(float p_delta, const VehicleInputState &p_input_state) {
}

String LNVehicleWheelShaft::get_debug_text() const {
	return "";
	//return vformat(String::utf8("Spring force: %.2f N\nSpring compression: %.2f\nDrive torque: %.2f N m\nSlip ratio: %.2f\nSlip angle: %.2fº"), suspension_state.spring_force, suspension_state.spring_displacement, drive_torque, wheel_state.slip_ratio, Math::rad_to_deg(wheel_state.slip_angle));
}

void LNVehicleWheelShaft::apply_arb(LNVehicle *p_vehicle, Ref<LNVehicleWheelShaft> p_other_wheel, float p_arb_stiffness) {
	if (!suspension_state.grounded || !p_other_wheel->suspension_state.grounded) {
		return;
	}

	const float arb_torque = p_arb_stiffness * ((-suspension_state.compression) - (-p_other_wheel->suspension_state.compression));

	// Use contact normals, consistent with spring force
	LNPhysics::apply_force(p_vehicle, -suspension_state.contact_normal * arb_torque, suspension_state.ground_hit_position - p_vehicle->get_global_position(), Color(1.0, 0.0, 0.0));
	LNPhysics::apply_force(p_vehicle, p_other_wheel->suspension_state.contact_normal * arb_torque, p_other_wheel->suspension_state.ground_hit_position - p_vehicle->get_global_position(), Color(1.0, 0.0, 0.0));
}

LNVehicleShaft::UpstreamData LNVehicleWheelShaft::get_upstream_data() {
	return {
		.inertia = wheel_state.wheel_inertia,
		.angular_velocity = wheel_state.angular_velocity,
		.net_reaction_torque = wheel_state.net_reaction_torque
	};
}

Transform3D LNVehicleWheelShaft::get_suspension_transform(const LNVehicle *p_vehicle_node, const LNVehicleWheel *p_wheel_node) const {
	const float wheel_sign = p_wheel_node->get_wheel_position() == WHEEL_RL || p_wheel_node->get_wheel_position() == WHEEL_FL ? 1.0f : -1.0f;

	if (!suspension_trf_vehicle_local.has_value()) {
		suspension_trf_vehicle_local = Transform3D();
		suspension_trf_vehicle_local->basis = Basis().scaled(Vector3(wheel_sign, 1.0, 1.0));
		suspension_trf_vehicle_local->origin = p_vehicle_node->to_local(p_wheel_node->get_global_position());
	}

	return p_vehicle_node->get_global_transform() * (*suspension_trf_vehicle_local);
}

float LNVehicleWheelShaft::get_slip_ratio() const {
	return wheel_state.slip_ratio;
}

float LNVehicleWheelShaft::get_slip_angle() const {
	return wheel_state.slip_angle;
}
