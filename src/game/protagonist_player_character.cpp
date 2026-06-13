#include "protagonist_player_character.h"

#include "game/biped_animation_base.h"
#include "game/character_model.h"
#include "game/milk_animation.h"
#include "game/movement_settings.h"
#include "game/movement_shared.h"
#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/classes/packed_scene.hpp"
#include "godot_cpp/classes/resource_loader.hpp"

void PlayerCharacterProtagonist::_bind_methods() {
}

void PlayerCharacterProtagonist::_ready() {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	Ref<PackedScene> scene = ResourceLoader::get_singleton()->load("res://scenes/milk_model.tscn");
	CharacterModel *milk_model = Object::cast_to<CharacterModel>(scene->instantiate());
	attach_milk_model(milk_model);
	PlayerCharacter::_ready();
}

void PlayerCharacterProtagonist::attach_milk_model(CharacterModel *p_model) {
	add_child(p_model);
	p_model->set_as_top_level(true);
	milk_model = p_model;
	if (milk_animation == nullptr) {
		milk_animation = memnew(MilkAnimation);
	}
	Ref<MovementSettings> setts;
	setts.instantiate();
	milk_animation->initialize(setts, p_model);
	milk_animation->set_hanging(true);
}

CharacterModel *PlayerCharacterProtagonist::get_milk_model() const {
	return milk_model;
}

void PlayerCharacterProtagonist::_process(double p_delta) {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	PlayerCharacter::_process(p_delta);

	if (milk_model) {
		Vector3 current_pos_relative = milk_model->get_global_transform_interpolated().origin - model->get_global_position();

		Vector3 angular_vel = LNMath::quat_to_scaled_angle_axis(Quaternion(prev_milk_pos_relative_to_player, current_pos_relative)) / p_delta;

		prev_milk_pos_relative_to_player = current_pos_relative;

		milk_animation->set_carrier_angular_velocity(angular_vel);
		milk_animation->update(Movement::MovementSpeed::IDLING, p_delta);
	}
}

void PlayerCharacterProtagonist::_physics_process(double p_delta) {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	const Vector3 forward = Vector3(0.0, 0.0, -1.0f);
	PlayerCharacter::_physics_process(p_delta);

	milk_model->update(p_delta);
	milk_model->set_global_transform(get_milk_attachment_transform());
}

Vector3 PlayerCharacterProtagonist::get_firing_position(int p_weapon_slot) const {
	if (p_weapon_slot == WeaponSlot::WEAPON_SLOT_PRIMARY || !milk_model) {
		return BaseCharacter::get_firing_position(p_weapon_slot);
	}
	return milk_model->get_firing_position_node()->get_global_position();
}

PlayerCharacterProtagonist::~PlayerCharacterProtagonist() {
	if (milk_animation) {
		memdelete(milk_animation);
	}
}
