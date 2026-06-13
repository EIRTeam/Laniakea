#include "character_animation_settings.h"

#include "game/biped_animation_base.h"
#include "godot_cpp/core/error_macros.hpp"

static constexpr std::array animation_set_to_str = {
	"none",
	"rifle"
};

static_assert(std::size(animation_set_to_str) == BipedAnimationBase::WEAPON_ANIMATION_SET_TYPE_MAX);

static constexpr std::array weapon_animation_to_str = {
	"idle",
	"aim",
	"reload"
};

static_assert(std::size(weapon_animation_to_str) == BipedAnimationBase::CharacterWeaponAnimations::WEAPON_ANIMATION_MAX);

static constexpr std::array sequence_to_str = {
	"reload"
};

static_assert(std::size(sequence_to_str) == CharacterAnimationBase::CharacterAnimationSequence::ANIMATION_SEQUENCE_MAX);

void CharacterAnimationSettings::_bind_methods() {
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
		ERR_FAIL_INDEX_V(animation_idx, BipedAnimationBase::CharacterWeaponAnimations::WEAPON_ANIMATION_MAX, false);

		r_ret = weapon_animation_sets[set_idx].animations[animation_idx];
		return true;
	}

	if (p_name.begins_with("sequences/")) {
		String name = p_name;
		const String sequence_name = name.get_slicec('/', 1);
		const String sequence_idx = sequence_name.get_slicec('_', 0);
		ERR_FAIL_COND_V(!sequence_idx.is_valid_int(), false);

		r_ret = sequences[sequence_idx.to_int()].animation;
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
		ERR_FAIL_INDEX_V(animation_idx, BipedAnimationBase::CharacterWeaponAnimations::WEAPON_ANIMATION_MAX, false);

		weapon_animation_sets[set_idx].animations[animation_idx] = p_value;
		return true;
	}

	if (p_name.begins_with("sequences/")) {
		String name = p_name;
		const String sequence_name = name.get_slicec('/', 1);
		const String sequence_idx = sequence_name.get_slicec('_', 0);
		ERR_FAIL_COND_V(!sequence_idx.is_valid_int(), false);

		sequences[sequence_idx.to_int()].animation = p_value;
		return true;
	}

	return false;
}

void CharacterAnimationSettings::_get_property_list(List<PropertyInfo> *p_list) const {
	for (int i = 0; i < BipedAnimationBase::WeaponAnimationSetType::WEAPON_ANIMATION_SET_TYPE_MAX; i++) {
		const String anim_set_name = "weapon_animation_sets/" + itos(i) + "_" + animation_set_to_str[i] + "/";
		for (int j = 0; j < BipedCharacterUpperBodyState::UPPER_BODY_STATE_MAX; j++) {
			const String anim_name = itos(j) + "_" + weapon_animation_to_str[j];
			p_list->push_back(PropertyInfo(Variant::STRING_NAME, anim_set_name + anim_name));
		}
	}

	for (int i = 0; i < CharacterAnimationBase::ANIMATION_SEQUENCE_MAX; i++) {
		const String sequence_name = "sequences/" + itos(i) + "_" + sequence_to_str[i] + "/";
		p_list->push_back(PropertyInfo(Variant::STRING_NAME, sequence_name + String("animation")));
	}
}

StringName CharacterAnimationSettings::get_weapon_animation(const BipedAnimationBase::WeaponAnimationSetType p_set, const BipedAnimationBase::CharacterWeaponAnimations p_anim) const {
	ERR_FAIL_INDEX_V(p_set, BipedAnimationBase::WEAPON_ANIMATION_SET_TYPE_MAX, "");
	ERR_FAIL_INDEX_V(p_anim, BipedAnimationBase::CharacterWeaponAnimations::WEAPON_ANIMATION_MAX, "");
	return weapon_animation_sets[p_set].animations[p_anim];
}

StringName CharacterAnimationSettings::get_sequence_animation(const CharacterAnimationBase::CharacterAnimationSequence p_sequence) const {
	ERR_FAIL_INDEX_V(p_sequence, sequences.size(), "");
	return sequences[p_sequence].animation;
}

CharacterAnimationSettings::CharacterAnimationSettings() {
	for (int i = 0; i < BipedAnimationBase::WEAPON_ANIMATION_SET_TYPE_MAX; i++) {
		animation_set_map.insert(StringName(animation_set_to_str[i]), static_cast<BipedAnimationBase::WeaponAnimationSetType>(i));
	}
}
