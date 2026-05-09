#include "character_model.h"
#include "game/main_loop.h"
#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/classes/window.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/core/property_info.hpp"
#include "godot_cpp/variant/quaternion.hpp"
#include "animation/hip_rotator_modifier.h"

void CharacterModel::_bind_methods() {
    MAKE_BIND_NODE(CharacterModel, skeleton, Skeleton3D);
    MAKE_BIND_NODE(CharacterModel, animation_tree, AnimationTree);
    MAKE_BIND_NODE(CharacterModel, milk_attachment_point, Node3D);
    MAKE_BIND_NODE(CharacterModel, milk_hip_target, Node3D);
    MAKE_BIND_NODE(CharacterModel, firing_position_node, Node3D);
    MAKE_BIND_NODE(CharacterModel, hand_attachment_node, Node3D);
    MAKE_BIND_NODE(CharacterModel, hip_rotator, HipRotatorModifier3D);
    MAKE_BIND_NODE(CharacterModel, eye_position_node, Node3D);
    MAKE_BIND_NODE(CharacterModel, hitbox_detector, CharacterHitboxDetector);
    MAKE_BIND_NODE(CharacterModel, ragdoll_simulator, PhysicalBoneSimulator3D);
    MAKE_BIND_RESOURCE(CharacterModel, animation_settings, CharacterAnimationSettings);
}

void CharacterModel::update(float p_delta) {
    if (!target_facing_direction.is_normalized()) {
        return;
    }
    facing_spring.update(target_facing_direction.normalized(), p_delta);

    set_global_basis(facing_spring.get_value());
}

void CharacterModel::set_target_facing_direction(Vector3 p_facing_direction) {
    DEV_ASSERT(p_facing_direction.is_normalized());
    target_facing_direction = Basis::looking_at(p_facing_direction).get_rotation_quaternion();    
}

Vector3 CharacterModel::get_target_facing_direction() const {
    return target_facing_direction.xform(Vector3(0.0, 0.0, -1.0f));
}

Vector3 CharacterModel::get_eye_position() const {
    ERR_FAIL_NULL_V_MSG(eye_position_node, get_global_position(), "Tried to get eye position, but model doesn't have an eye position node");
    return eye_position_node->get_global_position();
}

void CharacterModel::notify_died(const Vector3 &p_last_hit_normal) {
    // Create a copy of our current pose and turn into a ragdoll
    CharacterModel *new_model = Object::cast_to<CharacterModel>(duplicate());
    // Remove now unused modifiers
    Skeleton3D* skel = new_model->get_skeleton();
    for (int i = 0; i < skel->get_child_count(); i++) {
        Node *child = skel->get_child(i);
        if (Object::cast_to<SkeletonModifier3D>(child) && !Object::cast_to<PhysicalBoneSimulator3D>(child)) {
            child->queue_free();
        }

        if (Object::cast_to<CharacterHitboxDetector>(child)) {
            child->queue_free();
        }
    }

    skel->set_owner(nullptr);

    PhysicalBoneSimulator3D *simulator = new_model->ragdoll_simulator;

    skel->get_parent()->remove_child(skel);
    LaniakeaMainLoop::get_singleton()->get_root()->add_child(skel);
    skel->set_global_transform(skeleton->get_global_transform());
    new_model->queue_free();

    if (simulator) {
        simulator->set_active(true);
        simulator->physical_bones_start_simulation();
    }
}
