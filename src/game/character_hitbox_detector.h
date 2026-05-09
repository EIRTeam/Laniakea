#pragma once

#include "game/damageable.h"
#include "godot_cpp/classes/node3d.hpp"
#include "godot_cpp/classes/skeleton3d.hpp"
#include "godot_cpp/classes/static_body3d.hpp"
#include "character_hitbox.h"

using namespace godot;

class CharacterHitboxDetector : public StaticBody3D, public IDamageable {
    GDCLASS(CharacterHitboxDetector, Node3D);
    Skeleton3D *skeleton = nullptr;
    Vector<CharacterHitbox *> hitboxes;
    void notify_hit(const CharacterHitbox::HitboxGroup p_category, const Vector3 &p_position, const Vector3 &p_normal, int p_ammo_type, float p_damage);
    HashMap<StringName, Transform3D> global_pose_cache;
public:
    virtual void _ready() override;
    void update_pose_cache();
    Skeleton3D *get_skeleton() const;

    static void _bind_methods();
    virtual void on_bullet_damage_received(int p_ammo_type, float p_damage, const Vector3 &p_position, const Vector3 &p_normal, int p_shape_idx) override;

    CharacterHitboxDetector();

    void _notification(int p_what);

    friend class CharacterHitbox;
};