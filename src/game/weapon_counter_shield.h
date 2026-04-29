#pragma once

#include "console/cvar.h"
#include "game/damageable.h"
#include "godot_cpp/classes/box_shape3d.hpp"
#include "godot_cpp/classes/sphere_mesh.hpp"
#include "weapon_instance.h"

using namespace godot;

namespace godot {
    class MeshInstance3D;
};

class WeaponCounterShield : public WeaponInstanceBase, public IDamageable {
    GDCLASS(WeaponCounterShield, WeaponInstanceBase);

    struct CollectedBullet {
        MeshInstance3D *mesh;
    };

    LocalVector<CollectedBullet> collected_bullets;

    const int MAX_BULLET_COUNT = 5;

    static CVar weapon_counter_shield_radius_cvar;
    static CVar weapon_counter_shield_offset_cvar;
    Vector3 _get_shield_center(BaseCharacter *p_character) const;

    struct ShieldVisuals {
        Ref<Tween> tween;
        Node3D *node = nullptr;
    } shield_visuals;


    RID shield_physics_body;
    Ref<BoxShape3D> shield_shape;
    Ref<SphereMesh> bullet_shape;
    void _update_shield_visuals(float p_animation_progress, BaseCharacter *p_character);
public:
    virtual void primary_attack(int p_weapon_slot, const WeaponButtonState &p_button_state, BaseCharacter *p_character) override;
    virtual void post_update(int p_weapon_slot, BaseCharacter *p_character, const WeaponButtonState &p_button_state) override;
    virtual float get_max_distance() const override;
    virtual StringName get_item_name() const override;
    static StringName _get_weapon_name();
    virtual bool uses_occluded_crosshair() const override;
    virtual void equipped(int p_weapon_slot, BaseCharacter *p_character) override;
    virtual void unequipped(int p_weapon_slot, BaseCharacter *p_character) override;
    virtual void on_bullet_damage_received(int p_ammo_type, float p_damage, const Vector3 &p_position, const Vector3 &p_normal, int p_shape_idx) override;
    static void _bind_methods() {}
};