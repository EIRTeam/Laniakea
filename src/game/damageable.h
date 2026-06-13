#pragma once

#include "godot_cpp/variant/vector3.hpp"

using namespace godot;

class IDamageable {
public:
	virtual void on_bullet_damage_received(int p_ammo_type, float p_damage, const Vector3 &p_position, const Vector3 &p_normal, int p_shape_idx) = 0;
};
