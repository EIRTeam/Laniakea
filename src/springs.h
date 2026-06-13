#pragma once

#include "godot_cpp/core/print_string.hpp"
#include "godot_cpp/variant/packed_vector3_array.hpp"
#include "godot_cpp/variant/quaternion.hpp"
#include "math.h"

using namespace godot;

namespace Springs {

static _FORCE_INLINE_ float halflife_to_damping(float halflife, float eps = 1e-5f) {
	return (4.0f * 0.69314718056f) / (halflife + eps);
}

static _FORCE_INLINE_ void simple_spring_damper_exact(
		float &x,
		float &v,
		float x_goal,
		float halflife,
		float dt) {
	float y = halflife_to_damping(halflife) / 2.0f;
	float j0 = x - x_goal;
	float j1 = v + j0 * y;
	float eydt = LNMath::fast_negexp(y * dt);

	x = eydt * (j0 + j1 * dt) + x_goal;
	v = eydt * (v - j1 * y * dt);
}

static _FORCE_INLINE_ void simple_spring_damper_exact_quat(
		Quaternion &x,
		Vector3 &v,
		const Quaternion &x_goal,
		float halflife,
		float dt) {
	float y = halflife_to_damping(halflife) / 2.0f;

	Vector3 j0 = LNMath::quat_to_scaled_angle_axis(LNMath::quat_abs(x * x_goal.inverse()));
	Vector3 j1 = v + j0 * y;

	float eydt = LNMath::fast_negexp(y * dt);

	x = LNMath::quat_from_scaled_angle_axis(eydt * (j0 + j1 * dt)) * x_goal;
	v = eydt * (v - j1 * y * dt);
}

static _FORCE_INLINE_ void decay_spring_damper_exact_to_zero(
		float &x,
		float &v,
		float halflife,
		float dt) {
	float y = halflife_to_damping(halflife) / 2.0f;
	float j1 = v + x * y;
	float eydt = LNMath::fast_negexp(y * dt);

	x = eydt * (x + j1 * dt);
	v = eydt * (v - j1 * y * dt);
}

static _FORCE_INLINE_ void spring_character_update(
		float &x,
		float &v,
		float &a,
		float v_goal,
		float halflife,
		float dt) {
	float y = halflife_to_damping(halflife) / 2.0f;
	float j0 = v - v_goal;
	float j1 = a + j0 * y;
	float eydt = LNMath::fast_negexp(y * dt);

	x = eydt * (((-j1) / (y * y)) + ((-j0 - j1 * dt) / y)) +
			(j1 / (y * y)) + j0 / y + v_goal * dt + x;
	v = eydt * (j0 + j1 * dt) + v_goal;
	a = eydt * (a - j1 * y * dt);
}

static _FORCE_INLINE_ void spring_character_predict(
		Vector3 px[],
		Vector3 pv[],
		Vector3 pa[],
		int count,
		Vector3 x,
		Vector3 v,
		Vector3 a,
		Vector3 v_goal,
		float halflife,
		float dt) {
	for (int i = 0; i < count; i++) {
		px[i] = x;
		pv[i] = v;
		pa[i] = a;
	}

	for (int i = 0; i < count; i++) {
		spring_character_update(px[i].x, pv[i].x, pa[i].x, v_goal.x, halflife, i * dt);
		spring_character_update(px[i].y, pv[i].y, pa[i].y, v_goal.y, halflife, i * dt);
		spring_character_update(px[i].z, pv[i].z, pa[i].z, v_goal.z, halflife, i * dt);
	}
}

static _FORCE_INLINE_ void decay_spring_damper_exact(
		Vector3 &x,
		Vector3 &v,
		const float halflife,
		const float dt) {
	float y = halflife_to_damping(halflife) / 2.0f;
	Vector3 j1 = v + x * y;
	float eydt = LNMath::fast_negexp(y * dt);

	x = eydt * (x + j1 * dt);
	v = eydt * (v - j1 * y * dt);
}

static inline void decay_spring_damper_exact(
		Quaternion &x,
		Vector3 &v,
		const float halflife,
		const float dt) {
	float y = halflife_to_damping(halflife) / 2.0f;

	Vector3 j0 = LNMath::quat_to_scaled_angle_axis(x);
	Vector3 j1 = v + j0 * y;

	float eydt = LNMath::fast_negexp(y * dt);

	x = LNMath::quat_from_scaled_angle_axis(eydt * (j0 + j1 * dt));
	v = eydt * (v - j1 * y * dt);
}

static inline void inertialize_transition(
		Vector3 &off_x,
		Vector3 &off_v,
		const Vector3 src_x,
		const Vector3 src_v,
		const Vector3 dst_x,
		const Vector3 dst_v) {
	off_x = (src_x + off_x) - dst_x;
	off_v = (src_v + off_v) - dst_v;
}

static inline void inertialize_update(
		Vector3 &out_x,
		Vector3 &out_v,
		Vector3 &off_x,
		Vector3 &off_v,
		const Vector3 in_x,
		const Vector3 in_v,
		const float halflife,
		const float dt) {
	decay_spring_damper_exact(off_x, off_v, halflife, dt);
	out_x = in_x + off_x;
	out_v = in_v + off_v;
}

static inline void inertialize_transition(
		Quaternion &off_x,
		Vector3 &off_v,
		const Quaternion &src_x,
		const Vector3 &src_v,
		const Quaternion &dst_x,
		const Vector3 &dst_v) {
	off_x = LNMath::quat_abs(LNMath::quat_mul(LNMath::quat_mul(off_x, src_x), LNMath::quat_inv(dst_x)));
	off_v = (off_v + src_v) - dst_v;
}

static inline void inertialize_update(
		Quaternion &out_x,
		Vector3 &out_v,
		Quaternion &off_x,
		Vector3 &off_v,
		const Quaternion &in_x,
		const Vector3 &in_v,
		const float halflife,
		const float dt) {
	decay_spring_damper_exact(off_x, off_v, halflife, dt);
	out_x = LNMath::quat_mul(off_x, in_x);
	out_v = off_v + off_x.xform(in_v);
}

struct QuaternionSpringCritical {
private:
	Quaternion x;
	Vector3 v;
	float halflife = 0.1f;

public:
	void update(const Quaternion &p_target, float p_delta) {
		/*Quaternion relative = p_target.inverse() * x;  // or x * x_goal.inverse(), depending on convention
		Quaternion target = p_target;
		if (relative.w < 0) {  // or dot(x, x_goal) < 0
			target = -p_target;  // flip to the equivalent quaternion on the same hemisphere
		}*/
		Springs::simple_spring_damper_exact_quat(x, v, p_target, halflife, p_delta);
	}

	Quaternion get_value() const {
		return x;
	}

	Vector3 get_velocity() {
		return v;
	}
	void set_velocity(const Vector3 &p_velocity) {
		v = p_velocity;
	}

	void reset(const Quaternion &p_new, bool p_reset_velocity = true) {
		x = p_new;
		v = p_reset_velocity ? Vector3() : v;
	}
};

struct OffsetSpringCritical {
private:
	Vector3 position;
	float v = 0.0f;
	float halflife = 0.1f;

public:
	void initialize(float p_halflife) {
		halflife = p_halflife;
	}
	void update(const Vector3 &p_target, float p_delta) {
		const Vector3 diff = position - p_target;
		float new_distance = diff.length();
		Springs::decay_spring_damper_exact_to_zero(new_distance, v, halflife, p_delta);
		position = p_target + (!Math::is_zero_approx(new_distance) ? diff.normalized() * new_distance : Vector3());
	}
	void reset(const Vector3 &p_new, bool p_reset_velocity = true) {
		position = p_new;
		v = p_reset_velocity ? 0.0f : v;
	}
	Vector3 get() const {
		return position;
	}
};

struct OffsetSpringCriticalVector2 {
private:
	Vector2 position;
	float v = 0.0f;
	float halflife = 0.1f;

public:
	void initialize(float p_halflife) {
		halflife = p_halflife;
	}
	void update(const Vector2 &p_target, float p_delta) {
		const Vector2 diff = position - p_target;
		float new_distance = diff.length();
		Springs::decay_spring_damper_exact_to_zero(new_distance, v, halflife, p_delta);
		position = p_target + (!Math::is_zero_approx(new_distance) ? diff.normalized() * new_distance : Vector2());
	}
	void reset(const Vector2 &p_new, bool p_reset_velocity = true) {
		position = p_new;
		v = p_reset_velocity ? 0.0f : v;
	}
	Vector2 get() const {
		return position;
	}
};

struct PositionSpringCritical {
private:
	Vector3 position;
	Vector3 acceleration;
	float halflife = 0.1f;

public:
	void initialize(float p_halflife) {
		halflife = p_halflife;
	}
	void update(const Vector3 &p_target, float p_delta) {
		simple_spring_damper_exact(position.x, acceleration.x, p_target.x, halflife, p_delta);
		simple_spring_damper_exact(position.y, acceleration.y, p_target.y, halflife, p_delta);
		simple_spring_damper_exact(position.z, acceleration.z, p_target.z, halflife, p_delta);
	}
	PackedVector3Array predict(const Vector3 &p_pos, const Vector3 &p_target, float p_delta, int p_steps) const {
		PackedVector3Array out;
		out.resize(p_steps);
		const float dt = p_delta / static_cast<float>(p_steps - 1);

		Vector3 *r_out_w = out.ptrw();

		Vector3 accel_temp;

		for (int i = 0; i < p_steps; i++) {
			accel_temp = acceleration;
			const float delta = dt * i;
			Vector3 out_velocity = position;
			r_out_w[i] = p_pos;
			spring_character_update(r_out_w[i].x, out_velocity.x, accel_temp.x, p_target.x, halflife, delta);
			spring_character_update(r_out_w[i].y, out_velocity.y, accel_temp.y, p_target.y, halflife, delta);
			spring_character_update(r_out_w[i].z, out_velocity.z, accel_temp.z, p_target.z, halflife, delta);
		}

		return out;
	}
	void reset(const Vector3 &p_new, bool p_reset_velocity = true) {
	}
	Vector3 get() const {
		return position;
	}
};

struct SpringCritical {
private:
	float x = 0.0f;
	float v = 0.0f;
	float halflife = 0.1f;

public:
	void initialize(float p_halflife) {
		halflife = p_halflife;
	}
	void update(float p_target, float p_delta) {
		simple_spring_damper_exact(
				x,
				v,
				p_target,
				halflife,
				p_delta);
	}
	void reset(const float p_new, bool p_reset_velocity = true) {
		v = p_reset_velocity ? 0.0f : v;
		x = p_new;
	}
	float get() const {
		return x;
	}
};

}; //namespace Springs
