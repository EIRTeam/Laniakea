#pragma once

#include "game/biped_animation_base.h"
#include "game/character_animation_base.h"
#include "godot_cpp/classes/resource.hpp"

using namespace godot;

class CharacterAnimationSettings : public Resource {
	GDCLASS(CharacterAnimationSettings, Resource);

	struct WeaponAnimationSet {
		std::array<StringName, BipedAnimationBase::CharacterWeaponAnimations::WEAPON_ANIMATION_MAX> animations;
	};

	struct SequenceConfig {
		StringName animation;
	};

	HashMap<StringName, BipedAnimationBase::WeaponAnimationSetType> animation_set_map;
	HashMap<StringName, BipedCharacterUpperBodyState> upper_body_animation_map;

	std::array<WeaponAnimationSet, BipedAnimationBase::WEAPON_ANIMATION_SET_TYPE_MAX> weapon_animation_sets;
	std::array<SequenceConfig, CharacterAnimationBase::ANIMATION_SEQUENCE_MAX> sequences;

public:
	static void _bind_methods();
	bool _get(const StringName &p_name, Variant &r_ret) const;
	bool _set(const StringName &p_name, const Variant &p_value);
	void _get_property_list(List<PropertyInfo> *p_list) const;
	StringName get_weapon_animation(const BipedAnimationBase::WeaponAnimationSetType p_animation_set, const BipedAnimationBase::CharacterWeaponAnimations p_animation) const;
	StringName get_sequence_animation(const CharacterAnimationBase::CharacterAnimationSequence p_sequence) const;
	CharacterAnimationSettings();
};
