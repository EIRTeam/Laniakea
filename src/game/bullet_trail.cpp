#include "bullet_trail.h"

#include "debug/debug_overlay.h"
#include "godot_cpp/classes/camera3d.hpp"
#include "godot_cpp/classes/node.hpp"
#include "godot_cpp/classes/resource_loader.hpp"
#include "godot_cpp/classes/shader_material.hpp"
#include "godot_cpp/classes/viewport.hpp"

void BulletTrail::_ready() {
	Ref<Mesh> trail_mesh = ResourceLoader::get_singleton()->load("res://models/bullet_trail.tres");
	set_mesh(trail_mesh);
	hide();
}

void BulletTrail::initialize(Vector3 p_origin, Vector3 p_target, float p_velocity) {
	origin = p_origin;
	const Vector3 diff = p_target - p_origin;
	distance = diff.length();
	origin_to_target_dir = distance != 0.0f ? diff / distance : Vector3();
	velocity = p_velocity;
	set_physics_interpolation_mode(PHYSICS_INTERPOLATION_MODE_OFF);
}

void BulletTrail::_process(double p_delta) {
	const float trail_length = 5.0f;
	time += p_delta;

	const float head_dist_unclamped = time * velocity;
	const float head_dist = CLAMP(head_dist_unclamped, 0.0f, distance);
	const float tail_dist_unclamped = head_dist_unclamped - trail_length;
	const float tail_dist = CLAMP(tail_dist_unclamped, 0.0f, distance);

	if (tail_dist_unclamped > distance) {
		queue_free();
		return;
	}

	const float actual_length = head_dist - tail_dist;
	if (head_dist < trail_length) {
		const float uv_start = (trail_length - head_dist) / trail_length;
		set_instance_shader_parameter("uv_x_min", uv_start);
	} else {
		set_instance_shader_parameter("uv_x_min", 0.0f);
	}

	if (head_dist_unclamped > distance) {
		const float uv_end = 1.0f - ((head_dist_unclamped - distance) / trail_length);
		set_instance_shader_parameter("uv_x_max", uv_end);
	} else {
		set_instance_shader_parameter("uv_x_max", 1.0f);
	}

	// Now, place the trail where we should
	const Vector3 trail_center = origin_to_target_dir * (tail_dist + actual_length * 0.5f) + origin;

	Camera3D *cam = get_viewport()->get_camera_3d();
	ERR_FAIL_NULL(cam);

	const Vector3 z = origin_to_target_dir;
	const Vector3 x = get_global_position().direction_to(cam->get_global_position());
	const Vector3 y = x.cross(z);

	if (x.is_zero_approx() || y.is_zero_approx() || z.is_zero_approx()) {
		// Leave as-is
		return;
	}

	set_global_basis(Basis(x.normalized(), y.normalized() * 0.1f, -z.normalized() * actual_length));
	set_global_position(trail_center);
	show();
}

void BulletTrail::_bind_methods() {}
