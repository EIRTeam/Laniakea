#pragma once

#include "bind_macros.h"
#include "game/character_hitbox_detector.h"
#include "godot_cpp/classes/aim_modifier3d.hpp"
#include "godot_cpp/classes/animation_tree.hpp"
#include "godot_cpp/classes/node3d.hpp"
#include "godot_cpp/classes/skeleton3d.hpp"
#include "springs.h"

using namespace godot;

class HipRotatorModifier3D;

class CharacterModel : public Node3D {
    GDCLASS(CharacterModel, Node3D);
    Skeleton3D *skeleton = nullptr;
    AnimationTree *animation_tree = nullptr;
    Node3D *milk_attachment_point = nullptr;
    Node3D *firing_position_node = nullptr;
    Node3D *milk_hip_target = nullptr;
    HipRotatorModifier3D *hip_rotator = nullptr;
    Node3D *hand_attachment_node = nullptr;
    Node3D *eye_position_node = nullptr;
    CharacterHitboxDetector *hitbox_detector = nullptr;
    PhysicalBoneSimulator3D *ragdoll_simulator = nullptr;
    InertializationSkeletonModifierPolynomial *inertializer = nullptr;

    Springs::QuaternionSpringCritical facing_spring;
    Quaternion target_facing_direction;
public:
    MAKE_SETTER_GETTER_VALUE(Skeleton3D*, skeleton, skeleton);
    MAKE_SETTER_GETTER_VALUE(AnimationTree*, animation_tree, animation_tree);
    MAKE_SETTER_GETTER_VALUE(Node3D*, milk_attachment_point, milk_attachment_point);
    MAKE_SETTER_GETTER_VALUE(Node3D*, milk_hip_target, milk_hip_target);
    MAKE_SETTER_GETTER_VALUE(Node3D *, firing_position_node, firing_position_node);
    MAKE_SETTER_GETTER_VALUE(Node3D *, hand_attachment_node, hand_attachment_node);
    MAKE_SETTER_GETTER_VALUE(Node3D *, eye_position_node, eye_position_node);
    MAKE_SETTER_GETTER_VALUE(HipRotatorModifier3D *, hip_rotator, hip_rotator);
    MAKE_SETTER_GETTER_VALUE(CharacterHitboxDetector *, hitbox_detector, hitbox_detector);
    MAKE_SETTER_GETTER_VALUE(PhysicalBoneSimulator3D *, ragdoll_simulator, ragdoll_simulator);
    MAKE_SETTER_GETTER_VALUE(InertializationSkeletonModifierPolynomial *, inertializer, inertializer);
    static void _bind_methods();
    void update(float p_delta);
    void set_target_facing_direction(Vector3 p_facing_direction);
    Vector3 get_target_facing_direction() const;
    Vector3 get_eye_position() const;

    void notify_died(const Vector3 &p_last_hit_normal);
};