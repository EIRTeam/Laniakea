#pragma once

#include "bind_macros.h"
#include "godot_cpp/classes/skeleton_modifier3d.hpp"
#include "springs.h"

using namespace godot;

class HipRotatorModifier3D : public SkeletonModifier3D {
	GDCLASS(HipRotatorModifier3D, SkeletonModifier3D);

public:
	int hips_bone_idx = -1;
	int spine_bone_idx = -1;
	float target_aim_angle = 0.0f;

	Vector3 rotation_offset;
	StringName hips_bone;
	StringName spine_bone;

	MAKE_SETTER_GETTER_VALUE(Vector3, rotation_offset, rotation_offset);
	MAKE_SETTER_GETTER_VALUE(StringName, hips_bone, hips_bone);
	MAKE_SETTER_GETTER_VALUE(StringName, spine_bone, spine_bone);
	MAKE_SETTER_GETTER_VALUE(float, target_aim_angle, target_aim_angle);

	Springs::SpringCritical spring;

	static void _bind_methods();
	virtual void _process_modification_with_delta(double p_delta) override;
	virtual void _validate_bone_names() override;
};
