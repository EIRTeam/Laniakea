#pragma once

#include "godot_cpp/classes/rigid_body3d.hpp"

using namespace godot;

class LaniakeaPhysicsProp : public RigidBody3D {
	GDCLASS(LaniakeaPhysicsProp, RigidBody3D);
	static void _bind_methods() {}
	LaniakeaPhysicsProp();
};
