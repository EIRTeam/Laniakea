#include "inertialization_skeleton_modifier_polynomial.h"
#include "godot_cpp/classes/skeleton3d.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/core/math.hpp"
#include "godot_cpp/core/print_string.hpp"
#include "godot_cpp/variant/quaternion.hpp"
#include "godot_cpp/variant/transform3d.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "math.h"



void InertializationSkeletonModifierPolynomial::_recreate_data_arrays() {
    current_frame_data.resize(bone_count);
    last_frame_data.resize(bone_count);
    inert_data.resize(bone_count);
}

void calculate_inertialization_constants(const float p_x0, const float p_v0, float &r_A, float &r_B, float &r_C, const float p_accel, float &rp_desired_blend_time) {

	const float bt_2 = rp_desired_blend_time * rp_desired_blend_time;
	const float bt_3 = bt_2 * rp_desired_blend_time;
	const float bt_4 = bt_3 * rp_desired_blend_time;
	const float bt_5 = bt_4 * rp_desired_blend_time;

    r_A = -((p_accel * bt_2 + 6.0 * p_v0 * rp_desired_blend_time + 12.0 * p_x0) / (2.0 * bt_5));
	r_B = (3.0 * p_accel * bt_2 + 16.0 * p_v0 * rp_desired_blend_time + 30.0 * p_x0) / (2.0 * bt_4);
	r_C = -((3.0 * p_accel * bt_2 + 12.0 * p_v0 * rp_desired_blend_time + 20.0 * p_x0) / (2.0 * bt_3));
}

void InertializationSkeletonModifierPolynomial::_begin_inertialization(double p_delta) {
    Skeleton3D *skel = get_skeleton();
    inertialization_time = 0.0f;
    int inertialized_bones = 0;
    for (int i = 0; i < bone_count; i++) {
        const Transform3D bone_trf = skel->get_bone_pose(i);
        {
            Vector3 x_0_v = current_frame_data[i].position - bone_trf.origin;
            const Vector3 last_frame_pos = last_frame_data[i].position;
            Vector3 x_m1_v = last_frame_pos - bone_trf.origin;
            
            if (x_0_v.is_zero_approx()) {
                inert_data[i].position_transition_duration = 0.0f;
            // only hip bone needs position inertialization, HACK this for now...
            } else if (i == 1) {
                float x_0 = x_0_v.length();
                
                float x_m1 = x_m1_v.dot(x_0_v/x_0);
                float v0 = (x_0-x_m1)/p_delta;

                if (v0 > 0.0f) {
                    v0 = 0.0f;
                }

                inert_data[i].offset = x_0_v;
                inert_data[i].velocity = v0;
                inert_data[i].position_transition_duration = 0.2f;
                inert_data[i].position_accel = (-8.0f * v0 * inert_data[i].position_transition_duration - 20.0f * x_0) / (inert_data[i].position_transition_duration * inert_data[i].position_transition_duration);

                if (inert_data[i].position_accel < 0.0f) {
                    inert_data[i].position_accel = 0.0f;
                }

                calculate_inertialization_constants(
                    x_0,
                    v0,
                    inert_data[i].position_A,
                    inert_data[i].position_B,
                    inert_data[i].position_C,
                    inert_data[i].position_accel,
                    inert_data[i].position_transition_duration
                );
            }
        }

        {
            const Quaternion target_rot = bone_trf.basis.get_rotation_quaternion();
            const Quaternion q0 = LNMath::quat_abs(current_frame_data[i].rotation * LNMath::quat_inv(target_rot)).normalized();
            const Quaternion q_m1 = LNMath::quat_abs(last_frame_data[i].rotation * LNMath::quat_inv(target_rot)).normalized();

            real_t x0_angle;
            Vector3 x0_axis;

            x0_axis = q0.get_axis();
            x0_angle = q0.get_angle();

            if (Math::is_zero_approx(x0_angle) || !Math::is_finite(x0_angle) || !x0_axis.is_finite() || !x0_axis.is_normalized()) {
                inert_data[i].rotation_transition_duration = 0.0f;
                continue;
            }

            // Swing twist decomposition
            float xm1 = 2.0f * Math::atan(Vector3(q_m1.x, q_m1.y, q_m1.z).dot(x0_axis) / q_m1.w);
            
            float v0 = (x0_angle - xm1) / p_delta;

            DEV_ASSERT(Math::is_finite(v0));

            if (v0 > 0.0f) {
                v0 = 0.0f;
            }

            inert_data[i].rotation_transition_duration = 0.1f;
            if (v0 != 0.0f) {
                float transition_max = MIN(inert_data[i].rotation_transition_duration, -5.0 * (x0_angle / v0));
                if (transition_max > 0.0f) {
                    inert_data[i].rotation_transition_duration = MIN(inert_data[i].rotation_transition_duration, transition_max);
                }
            }
            float a0 = (-8.0f * v0 * inert_data[i].rotation_transition_duration - 20.0 * x0_angle) / (inert_data[i].rotation_transition_duration * inert_data[i].rotation_transition_duration);
            if (a0 < 0.0f) {
                a0 = 0.0f;
            }

            inert_data[i].rot_offset_axis = x0_axis;
            inert_data[i].rot_offset_angle = x0_angle;
            inert_data[i].rot_velocity = v0;
            inert_data[i].rotation_accel = a0;
            DEV_ASSERT(Math::is_finite(v0));
            DEV_ASSERT(Math::is_finite(a0));

            inertialized_bones++;

            calculate_inertialization_constants(
                x0_angle,
                v0,
                inert_data[i].rotation_A,
                inert_data[i].rotation_B,
                inert_data[i].rotation_C,
                inert_data[i].rotation_accel,
                inert_data[i].rotation_transition_duration
            );

            DEV_ASSERT(Math::is_finite(inert_data[i].rotation_A));
            DEV_ASSERT(Math::is_finite(inert_data[i].rotation_B));
            DEV_ASSERT(Math::is_finite(inert_data[i].rotation_transition_duration));

        }
    }

    print_verbose("Inertialized ", inertialized_bones, " bones!");
    inertializing = true;
}

float compute_inertialization(const float p_A, const float p_B, const float p_C, const float p_v0, const float p_x0, const float p_accel, const float p_time) {
	const float bt_2 = p_time * p_time;
	const float bt_3 = bt_2 * p_time;
	const float bt_4 = bt_3 * p_time;
	const float bt_5 = bt_4 * p_time;
    return p_A * bt_5 + p_B * bt_4 + p_C * bt_3 + 0.5f * p_accel * bt_2 + p_v0 * p_time + p_x0;
}

void InertializationSkeletonModifierPolynomial::_process_inertialization(double p_delta) {
    inertialization_time += p_delta;
    Skeleton3D *skel = get_skeleton();
    inertializing = false;
    for (int i = 0; i < bone_count; i++) {
        const InertializationData &bone_data = inert_data[i];
        Transform3D bone_trf = skel->get_bone_pose(i);
        const bool needs_to_inertialize_position = inertialization_time < bone_data.position_transition_duration;
        const bool needs_to_inertialize_rotation = inertialization_time < bone_data.rotation_transition_duration;

        if (needs_to_inertialize_rotation) {
            const float new_angle = compute_inertialization(
                bone_data.rotation_A,
                bone_data.rotation_B,
                bone_data.rotation_C,
                bone_data.rot_velocity,
                bone_data.rot_offset_angle,
                bone_data.rotation_accel,
                inertialization_time
            );

            const Quaternion diff = LNMath::quat_from_angle_axis(new_angle, bone_data.rot_offset_axis);
            const Quaternion new_rot = diff * bone_trf.basis.get_rotation_quaternion();
            bone_trf.basis = new_rot;
        }
        if (needs_to_inertialize_position) {
            const float xt = compute_inertialization(
                bone_data.position_A,
                bone_data.position_B,
                bone_data.position_C,
                bone_data.velocity,
                bone_data.offset.length(),
                bone_data.position_accel,
                inertialization_time
            );

            bone_trf.origin += xt * bone_data.offset.normalized();
        }

        if (needs_to_inertialize_position || needs_to_inertialize_rotation) {
            skel->set_bone_pose(i, bone_trf);
            inertializing = true;
        }
    }
}

void InertializationSkeletonModifierPolynomial::_update_data_arrays(double p_delta) {
    Skeleton3D *skel = get_skeleton();
    
    last_frame_data = current_frame_data;

    for (int i = 0; i < skel->get_bone_count(); i++) {
        const Transform3D bone_trf = skel->get_bone_pose(i);
        current_frame_data[i].position = bone_trf.origin;
        current_frame_data[i].rotation = bone_trf.basis.get_rotation_quaternion();
    }
}

void InertializationSkeletonModifierPolynomial::_run_process(double p_delta)
{
    if (bone_count != get_skeleton()->get_bone_count()) {
        bone_count = get_skeleton()->get_bone_count();
        _recreate_data_arrays();
    }

    if (inertialization_queued) {
        _begin_inertialization(p_delta);
        _update_data_arrays(p_delta);
        _process_inertialization(p_delta);
        inertialization_queued = false;
    } else if (inertializing) {
        _process_inertialization(p_delta);
        _update_data_arrays(p_delta);
    } else {
        _update_data_arrays(p_delta);
    }
}

int InertializationSkeletonModifierPolynomial::get_current_frame_pose_bone_count() const {
    return current_frame_data.size();    
}

Transform3D InertializationSkeletonModifierPolynomial::get_current_frame_pose_bone_pose(int p_idx) const {
    ERR_FAIL_INDEX_V(p_idx, current_frame_data.size(), Transform3D());
    return Transform3D(current_frame_data[p_idx].rotation, current_frame_data[p_idx].position);
}

void InertializationSkeletonModifierPolynomial::_bind_methods() {
    ClassDB::bind_method(D_METHOD("queue_inertialization"), &InertializationSkeletonModifierPolynomial::queue_inertialization);
}

void InertializationSkeletonModifierPolynomial::queue_inertialization() {
    inertialization_queued = true;
}

void InertializationSkeletonModifierPolynomial::_process_modification_with_delta(double p_delta) {
    _run_process(p_delta);
}
