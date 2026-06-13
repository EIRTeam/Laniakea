#include "hip_rotator_modifier.h"

#include "bind_macros.h"
#include "godot_cpp/classes/skeleton3d.hpp"

void HipRotatorModifier3D::_bind_methods() {
	MAKE_BIND_STRING_NAME(HipRotatorModifier3D, hips_bone);
	MAKE_BIND_STRING_NAME(HipRotatorModifier3D, spine_bone);
	MAKE_BIND_FLOAT(HipRotatorModifier3D, target_aim_angle);
	MAKE_BIND_VECTOR3_DEGREES(HipRotatorModifier3D, rotation_offset);
}

void HipRotatorModifier3D::_process_modification_with_delta(double p_delta) {
	if (get_skeleton() == nullptr || spine_bone_idx == -1 || hips_bone_idx == -1) {
		return;
	}

	spring.update(target_aim_angle, p_delta);

	Skeleton3D *skel = get_skeleton();
	const Transform3D global_pose_spine_rest = skel->get_bone_global_rest(spine_bone_idx);
	const Transform3D global_pose_spine = skel->get_bone_global_pose(spine_bone_idx);

	const Transform3D global_pose_hip_rest = skel->get_bone_global_rest(hips_bone_idx);
	const Transform3D global_pose_hip = skel->get_bone_global_pose(hips_bone_idx);

	const Transform3D diff = global_pose_hip * global_pose_hip_rest.affine_inverse();
	Transform3D new_global_spine = diff.inverse() * global_pose_spine;
	const Vector3 right = Vector3(1.0f, 0.0f, 0.0f);
	new_global_spine.basis = new_global_spine.basis.rotated(right, -spring.get()) * Quaternion::from_euler(rotation_offset);
	new_global_spine.origin = global_pose_spine.origin;

	skel->set_bone_global_pose(spine_bone_idx, new_global_spine);
}

void HipRotatorModifier3D::_validate_bone_names() {
	hips_bone_idx = -1;
	spine_bone_idx = -1;

	if (get_skeleton() == nullptr) {
		return;
	}

	hips_bone_idx = get_skeleton()->find_bone(hips_bone);
	spine_bone_idx = get_skeleton()->find_bone(spine_bone);
}
