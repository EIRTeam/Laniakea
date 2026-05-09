#pragma once

#include "game/biped_animation_base.h"
#include "godot_cpp/classes/ref_counted.hpp"
#include "base_character.h"

using namespace godot;

class BaseCharacter;
class WeaponModel;

class WeaponInstanceBase : public RefCounted {
    GDCLASS(WeaponInstanceBase, RefCounted);
public:
    struct WeaponButtonState {
        bool fire = false;
    };
    static void _bind_methods();
    virtual void equipped(int p_weapon_slot, BaseCharacter *p_character) {};
    virtual void unequipped(int p_weapon_slot, BaseCharacter *p_character) {};
    virtual void post_update(int p_weapon_slot, BaseCharacter *p_character, const WeaponButtonState &p_button_state) {}
    virtual void primary_attack(int p_weapon_slot, const WeaponButtonState &p_button_state, BaseCharacter *p_character) = 0;
    virtual bool uses_occluded_crosshair() const;
    virtual float get_max_distance() const {
        return 1.0f;
    }

    virtual WeaponModel *instantiate_visuals() const { return nullptr; };
    virtual BipedAnimationBase::WeaponAnimationSetType get_weapon_animation_type() const { return BipedAnimationBase::WEAPON_ANIMATION_TYPE_NONE; };
    virtual StringName get_item_name() const = 0;
    StringName get_weapon_name() const;
};