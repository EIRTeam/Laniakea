#include "player_camera.h"

#include "debug/debug_overlay.h"
#include "gdextension_interface.h"
#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/classes/input.hpp"
#include "godot_cpp/classes/input_event.hpp"
#include "godot_cpp/classes/node.hpp"
#include "godot_cpp/classes/viewport.hpp"
#include "godot_cpp/core/math.hpp"
#include "godot_cpp/core/math_defs.hpp"
#include "godot_cpp/core/print_string.hpp"

CVar PlayerCamera::camera_sensitivity = CVar::create_variable("camera.senstivity", GDEXTENSION_VARIANT_TYPE_FLOAT, 0.01f, "Camera sensitivity", PROPERTY_HINT_NONE, "");
CVar PlayerCamera::camera_sensitivity_mouse = CVar::create_variable("camera.senstivity_mouse", GDEXTENSION_VARIANT_TYPE_FLOAT, 0.01f, "Camera sensitivity", PROPERTY_HINT_NONE, "");
CVar PlayerCamera::camera_power = CVar::create_variable("camera.power", GDEXTENSION_VARIANT_TYPE_FLOAT, 1.0f, "Camera power factor", PROPERTY_HINT_NONE, "");
CVar PlayerCamera::camera_leash_multiplier = CVar::create_variable("camera.leash_multiplier", GDEXTENSION_VARIANT_TYPE_FLOAT, 1.0f, "Camera leash multiplier", PROPERTY_HINT_NONE, "");

void PlayerCamera::_bind_methods() {
	ClassDB::bind_method(D_METHOD("update", "target_velocity", "target_position"), &PlayerCamera::update);
}

void PlayerCamera::_unhandled_input(const Ref<InputEvent> &p_event) {
	Ref<InputEventMouseMotion> mouse_mot = p_event;

	if (!mouse_mot.is_valid()) {
		return;
	}
	mouse_relative_accum += mouse_mot->get_relative();
	get_viewport()->set_input_as_handled();
}

void PlayerCamera::update(Vector3 p_target_velocity, Vector3 p_target_position) {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}

	const float delta = get_process_delta_time();

	y_spring.update(p_target_position.y, delta);

	Input *input = Input::get_singleton();

	static const StringName camera_left = "camera_left";
	static const StringName camera_right = "camera_right";
	static const StringName camera_up = "camera_up";
	static const StringName camera_down = "camera_down";

	Vector2 input_vector = input->get_vector(camera_left, camera_right, camera_up, camera_down);
	input_vector.x = SIGN(input_vector.x) * Math::abs(pow(input_vector.x, camera_power.get_float()));
	input_vector.y = SIGN(input_vector.y) * Math::abs(pow(input_vector.y, camera_power.get_float()));

	pitch += -input_vector.y * camera_sensitivity.get_float() * delta;
	yaw += -input_vector.x * camera_sensitivity.get_float() * delta;

	if (mouse_relative_accum != Vector2()) {
		pitch -= mouse_relative_accum.y * camera_sensitivity_mouse.get_float();
		yaw -= mouse_relative_accum.x * camera_sensitivity_mouse.get_float();
		mouse_relative_accum = Vector2();
	}

	// deadzone
	const float horizontal_distance = ((get_global_position() - p_target_position) * Vector3(1.0f, 0.0f, 1.0f)).length();
	const bool was_outside_deadzone = horizontal_distance >= horizontal_deadzone_radius;

	const Vector3 dir_from_target = p_target_position.direction_to(get_global_position());
	if (was_outside_deadzone) {
		set_global_position(dir_from_target * horizontal_deadzone_radius + p_target_position);
	}

	{
		// update global pos
		Vector3 global_pos = get_global_position();
		global_pos.y = y_spring.get();
		set_global_position(global_pos);
	}

	// leash
	if (enable_leash && horizontal_distance > horizontal_deadzone_radius * 0.75f) {
		const Vector3 view_dir = -camera->get_global_basis().get_column(2);
		const Vector3 dir_to_vel_cross = view_dir.cross(p_target_velocity);
		if (dir_to_vel_cross.is_finite()) {
			yaw += dir_to_vel_cross.y * camera_leash_multiplier.get_float() * delta;
		}
	}

	pitch = CLAMP(pitch, -Math::deg_to_rad(89.9f), Math::deg_to_rad(89.9f));
	yaw = Math::wrapf(yaw, -Math_PI, Math_PI);

	const Vector3 up = Vector3(0.0, 1.0, 0.0);

	// Apply kick and rotation
	{
		punch_spring.update(Quaternion(), delta);

		const Vector3 kick_euler = punch_spring.get_value().get_euler();
		const float pitch_with_kick = CLAMP(pitch + kick_euler.x, -Math::deg_to_rad(89.9f), Math::deg_to_rad(89.9f));
		const float yaw_with_kick = Math::wrapf(yaw + kick_euler.y, -Math_PI, Math_PI);
		const float roll_with_kick = CLAMP(kick_euler.z, -Math_PI - 0.01f, Math_PI + 0.01f);

		inner_dummy->set_rotation(Vector3(pitch_with_kick, yaw_with_kick, roll_with_kick));
	}
	// Camera offset

	const float tan_fov_y = tan(0.5f * Math::deg_to_rad(camera->get_fov()));
	const float tan_fov_x = tan_fov_y * camera->get_camera_projection().get_aspect();

	framing_spring.update(framing, delta);
	distance_spring.update(distance, delta);

	const Vector3 local_offset = Vector3(
			distance_spring.get() * tan_fov_x * framing_spring.get().x,
			distance_spring.get() * tan_fov_y * framing_spring.get().y,
			distance_spring.get());

	DebugOverlay::horz_circle(get_global_position(), horizontal_deadzone_radius, Color(1.0f, 0.0f, 0.0f, 0.0f));

	camera->set_position(local_offset);
}

Vector2 PlayerCamera::transform_input(Vector2 p_input) const {
	const Vector3 forward = camera->get_global_basis().get_column(2);
	const Vector3 right = camera->get_global_basis().get_column(0);
	const Vector2 forward_2d = Vector2(forward.x, forward.z).normalized();
	const Vector2 right_2d = Vector2(right.x, right.z).normalized();
	Vector2 out;
	if (forward_2d.is_normalized()) {
		out += forward_2d * p_input.y;
	}
	if (right_2d.is_normalized()) {
		out += right_2d * p_input.x;
	}
	return out;
}

Camera3D *PlayerCamera::get_camera() const {
	return camera;
}

void PlayerCamera::set_framing(Vector2 p_new_framing, bool p_reset) {
	framing = p_new_framing;
	if (p_reset) {
		framing_spring.reset(p_new_framing);
	}
}

void PlayerCamera::set_distance(float p_new_distance, bool p_reset) {
	distance = p_new_distance;
	if (p_reset) {
		distance_spring.reset(p_new_distance);
	}
}

bool PlayerCamera::get_enable_leash() const {
	return enable_leash;
}

void PlayerCamera::set_enable_leash(bool p_enable_leash) {
	enable_leash = p_enable_leash;
}

void PlayerCamera::_ready() {
	distance_spring.reset(distance);
	framing_spring.reset(framing);
}

PlayerCamera::PlayerCamera() {
	inner_dummy = memnew(Node3D);
	camera = memnew(Camera3D);

	add_child(inner_dummy);
	inner_dummy->add_child(camera);
	set_physics_interpolation_mode(PHYSICS_INTERPOLATION_MODE_OFF);

	if (!Engine::get_singleton()->is_editor_hint()) {
	}
}

float PlayerCamera::get_horizontal_deadzone_radius() const {
	return horizontal_deadzone_radius;
}

void PlayerCamera::set_horizontal_deadzone_radius(float p_horizontal_deadzone_radius) {
	horizontal_deadzone_radius = p_horizontal_deadzone_radius;
}

void PlayerCamera::view_punch(Vector3 p_angular_velocity) {
	punch_spring.set_velocity(punch_spring.get_velocity() + p_angular_velocity);
}

void PlayerCamera::view_punch_reset(float p_angle_threshold_rads) {
	if (Quaternion().angle_to(punch_spring.get_value()) > p_angle_threshold_rads) {
		punch_spring.reset(Quaternion(), true);
	}
}

void PlayerCamera::do_machine_gun_kick(float p_max_vertical_kick_angle, float p_fire_duration_time, float p_slide_limit_time) {
	view_punch_reset(10.0f);

	constexpr float KICK_MIN_X = 0.2f;
	constexpr float KICK_MIN_Y = 0.1f;
	constexpr float KICK_MIN_Z = 0.2f;

	const float duration = MIN(p_fire_duration_time, p_slide_limit_time);
	const float kick_percentage = duration / p_slide_limit_time;

	Vector3 kick_angular = Vector3(
			-(KICK_MIN_X + (p_max_vertical_kick_angle * kick_percentage)),
			-(KICK_MIN_Y + (p_max_vertical_kick_angle * kick_percentage)) / 3.0f,
			(KICK_MIN_Z + (p_max_vertical_kick_angle * kick_percentage)) / 8.0f);

	// Left-down wobble
	if (UtilityFunctions::randi_range(-1, 1) >= 0) {
		kick_angular.y *= -1.0f;
	}

	// Up and down wobble
	if (UtilityFunctions::randi_range(-1, 1) >= 0) {
		kick_angular.x *= -1.0f;
	}

	view_punch(kick_angular * 5.0f);
}
