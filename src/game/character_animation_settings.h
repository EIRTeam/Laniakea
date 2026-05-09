#pragma once

#include "game/biped_animation_base.h"
#include "godot_cpp/classes/resource.hpp"

using namespace godot;

class CharacterAnimationSettings : public Resource {
    GDCLASS(CharacterAnimationSettings, Resource);

    struct WeaponAnimationSet {
        std::array<StringName, BipedAnimationBase::UpperBodyAnimationState::UPPER_BODY_STATE_MAX> upper_body_animations;
    };
    HashMap<StringName, BipedAnimationBase::WeaponAnimationSetType> animation_set_map;
    HashMap<StringName, BipedAnimationBase::UpperBodyAnimationState> upper_body_animation_map;

    std::array<WeaponAnimationSet, BipedAnimationBase::WEAPON_ANIMATION_SET_TYPE_MAX> weapon_animation_sets;
public:
    static void _bind_methods();
    bool _get(const StringName &p_name, Variant &r_ret) const;
    bool _set(const StringName &p_name, const Variant &p_value);
    void _get_property_list(List<PropertyInfo> *p_list) const;
    StringName get_animation(const BipedAnimationBase::WeaponAnimationSetType, const BipedAnimationBase::UpperBodyAnimationState) const;
    CharacterAnimationSettings();
};