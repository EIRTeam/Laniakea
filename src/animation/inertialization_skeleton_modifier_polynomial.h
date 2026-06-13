#pragma once

#include "godot_cpp/classes/skeleton_modifier3d.hpp"

#include <vector>

using namespace godot;
class InertializationSkeletonModifierPolynomial : public SkeletonModifier3D {
	GDCLASS(InertializationSkeletonModifierPolynomial, SkeletonModifier3D);

	struct InertializationData {
		float position_accel;
		float position_A;
		float position_B;
		float position_C;
		float rotation_accel;
		float rotation_A;
		float rotation_B;
		float rotation_C;

		Vector3 rot_offset_axis;
		float rot_offset_angle;
		float rot_offset_angle_dbg;
		float rot_velocity;

		Vector3 offset;
		float velocity;
		float position_transition_duration = 0.0f;
		float rotation_transition_duration = 0.0f;
	};

	struct BoneData {
		Vector3 position;
		Quaternion rotation;
	};

	LocalVector<BoneData> last_frame_data;
	LocalVector<BoneData> current_frame_data;
	LocalVector<InertializationData> inert_data;

	int bone_count = 0;

	bool inertializing = false;
	float inertialization_time = 0.0f;

	bool inertialization_queued = false;
	bool needs_velocity_calculations = false;
	void _recreate_data_arrays();
	void _begin_inertialization(double p_delta);
	void _process_inertialization(double p_delta);
	void _update_data_arrays(double p_delta);

public:
	void _run_process(double p_delta);
	int get_current_frame_pose_bone_count() const;
	Transform3D get_current_frame_pose_bone_pose(int p_idx) const;
	static void _bind_methods();
	void queue_inertialization();
	virtual void _process_modification_with_delta(double p_delta) override;
};
