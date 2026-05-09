#include "character_animation_settings.h"
#include "game/biped_animation_base.h"
#include "godot_cpp/core/error_macros.hpp"

static constexpr std::array animation_set_to_str = {
    "none",
    "rifle"
};

static_assert(std::size(animation_set_to_str) == BipedAnimationBase::WEAPON_ANIMATION_SET_TYPE_MAX);

static constexpr std::array upper_body_animation_state_to_str = {
    "idle",
    "aim",
    "reload"
};

static_assert(std::size(upper_body_animation_state_to_str) == BipedAnimationBase::UPPER_BODY_STATE_MAX);

void CharacterAnimationSettings::_bind_methods()
{
    
}

bool CharacterAnimationSettings::_get(const StringName &p_name, Variant &r_ret) const {
    String name = p_name;
    if (name.begins_with("weapon_animation_sets/")) {
        const String animation_set_name = name.get_slicec('/', 1);
        const String set_idx_str = animation_set_name.get_slicec('_', 0);

        ERR_FAIL_COND_V(!set_idx_str.is_valid_int(), false);
        
        const int set_idx = set_idx_str.to_int();
        ERR_FAIL_INDEX_V(set_idx, BipedAnimationBase::WEAPON_ANIMATION_SET_TYPE_MAX, false);

        const String animation_name = name.get_slicec('/', 2);
        const String animation_name_idx = animation_name.get_slicec('_', 0);

        ERR_FAIL_COND_V(!animation_name_idx.is_valid_int(), false);
        const int animation_idx = animation_name_idx.to_int();
        ERR_FAIL_INDEX_V(animation_idx, BipedAnimationBase::UPPER_BODY_STATE_MAX, false);
        
        r_ret = weapon_animation_sets[set_idx].upper_body_animations[animation_idx];
        return true;
    }

    return false;
}

bool CharacterAnimationSettings::_set(const StringName &p_name, const Variant &p_value) {
    String name = p_name;
    if (name.begins_with("weapon_animation_sets/")) {
        const String animation_set_name = name.get_slicec('/', 1);
        const String set_idx_str = animation_set_name.get_slicec('_', 0);

        ERR_FAIL_COND_V(!set_idx_str.is_valid_int(), false);
        
        const int set_idx = set_idx_str.to_int();
        ERR_FAIL_INDEX_V(set_idx, BipedAnimationBase::WEAPON_ANIMATION_SET_TYPE_MAX, false);

        const String animation_name = name.get_slicec('/', 2);
        const String animation_name_idx = animation_name.get_slicec('_', 0);

        ERR_FAIL_COND_V(!animation_name_idx.is_valid_int(), false);
        const int animation_idx = animation_name_idx.to_int();
        ERR_FAIL_INDEX_V(animation_idx, BipedAnimationBase::UPPER_BODY_STATE_MAX, false);
        
        weapon_animation_sets[set_idx].upper_body_animations[animation_idx] = p_value;
        return true;
    }
    return false;
}

void CharacterAnimationSettings::_get_property_list(List<PropertyInfo> *p_list) const {
    for (int i = 0; i < BipedAnimationBase::WeaponAnimationSetType::WEAPON_ANIMATION_SET_TYPE_MAX; i++) {
        const String anim_set_name = "weapon_animation_sets/" + itos(i) + "_" + animation_set_to_str[i] + "/";
        for (int j = 0; j < BipedAnimationBase::UPPER_BODY_STATE_MAX; j++) {
            const String anim_name = itos(j) + "_" + upper_body_animation_state_to_str[j];
            p_list->push_back(PropertyInfo(Variant::STRING_NAME, anim_set_name + anim_name));
        }
    }
}

StringName CharacterAnimationSettings::get_animation(const BipedAnimationBase::WeaponAnimationSetType p_set, const BipedAnimationBase::UpperBodyAnimationState p_anim) const {
    ERR_FAIL_INDEX_V(p_set, BipedAnimationBase::WEAPON_ANIMATION_SET_TYPE_MAX, "");
    ERR_FAIL_INDEX_V(p_anim, BipedAnimationBase::UPPER_BODY_STATE_MAX, "");
    return weapon_animation_sets[p_set].upper_body_animations[p_anim];
}

CharacterAnimationSettings::CharacterAnimationSettings() {
    for (int i = 0; i < BipedAnimationBase::WEAPON_ANIMATION_SET_TYPE_MAX; i++) {
        animation_set_map.insert(StringName(animation_set_to_str[i]), static_cast<BipedAnimationBase::WeaponAnimationSetType>(i));
    }
}
