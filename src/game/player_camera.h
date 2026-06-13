#pragma once

#include "console/cvar.h"
#include "godot_cpp/classes/camera3d.hpp"
#include "godot_cpp/classes/input_event_mouse_motion.hpp"
#include "godot_cpp/classes/ref_counted.hpp"
#include "springs.h"

using namespace godot;

// Camera will only be used by players, so it's fine if we use godot's input stuff.
class PlayerCamera : public Node3D {
	GDCLASS(PlayerCamera, Node3D);
	static CVar camera_sensitivity;
	static CVar camera_sensitivity_mouse;
	static CVar camera_power;
	static CVar camera_leash_multiplier;
	Node3D *inner_dummy = nullptr;
	Camera3D *camera = nullptr;
	float pitch = 0.0f;
	float yaw = 0.0f;
	float distance = 1.5f;
	Vector2 framing = Vector2(0.2, 0.0);

	Springs::OffsetSpringCriticalVector2 framing_spring;
	Springs::SpringCritical distance_spring;

	Springs::SpringCritical y_spring;

	Vector2 mouse_relative_accum;

	// leash
	bool enable_leash = true;
	float horizontal_deadzone_radius = 0.0f;

	Springs::QuaternionSpringCritical punch_spring;

public:
	static void _bind_methods();
	virtual void _unhandled_input(const Ref<InputEvent> &p_event) override;
	void update(Vector3 p_target_velocity, Vector3 p_target_position);
	Vector2 transform_input(Vector2 p_input) const;
	Camera3D *get_camera() const;
	void set_framing(Vector2 p_new_framing, bool p_reset);
	void set_distance(float p_new_distance, bool p_reset);
	virtual void _ready() override;
	PlayerCamera();

	bool get_enable_leash() const;
	void set_enable_leash(bool p_enable_leash);

	float get_horizontal_deadzone_radius() const;
	void set_horizontal_deadzone_radius(float p_horizontal_deadzone_radius);
	void view_punch(Vector3 p_angular_velocity);
	void view_punch_reset(float p_angle_threshold_rads);
	void do_machine_gun_kick(float p_max_vertical_kick_angle, float p_fire_duration_time, float p_slide_limit_time);
};
