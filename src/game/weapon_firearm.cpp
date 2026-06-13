#include "weapon_firearm.h"

#include "game/character_animation_base.h"
#include "game/game_rules_laniakea.h"
#include "game/main_loop.h"
#include "game/player_character.h"
#include "game/protagonist_player_character.h"

void WeaponFirearmInstance::_bind_methods() {
}

void WeaponFirearmInstance::get_aim_trajectory(int p_weapon_slot, BaseCharacter *p_character, Vector3 &r_origin, Vector3 &r_direction) const {
	return p_character->get_aim_trajectory(p_weapon_slot, r_origin, r_direction);
}

void WeaponFirearmInstance::post_update(int p_weapon_slot, BaseCharacter *p_character, const WeaponButtonState &p_button_state) {
	const float time = LaniakeaMainLoop::get_singleton()->get_physics_time();

	if (reload_future.is_valid() && reload_future->status == AnimationSequenceFuture::FINISHED && next_possible_primary_attack <= time) {
		reload_future.unref();
		// Reload complete, we can now fire!
		const int clip_capacity = get_clip_capacity();

		p_character->reload_weapon(p_weapon_slot, this, get_ammo_type(), get_clip_capacity());
	}

	fire_duration = p_button_state.fire ? fire_duration + p_character->get_physics_process_delta_time() : 0.0f;

	if (p_button_state.fire && next_possible_primary_attack <= time) {
		if (!has_ammo_in_clip(p_character)) {
			if (p_character->get_remaining_ammo_in_pool(get_ammo_type()) > 0) {
				const bool are_we_player = Object::cast_to<PlayerCharacter>(p_character) != nullptr;
				if (are_we_player) {
					reload_future = p_character->trigger_sequence(CharacterAnimationBase::SEQUENCE_RELOAD);
					next_possible_primary_attack = time + reload_future->duration;
					return;
				}
			} else {
				// TODO: Trigger empty weapon sound
			}
			return;
		}
		switch (get_fire_mode()) {
			case SEMI_AUTO: {
				fire_bullet(p_weapon_slot, p_character);
			} break;
			case FULL_AUTO: {
				while (p_button_state.fire && next_possible_primary_attack <= time && has_ammo_in_clip(p_character)) {
					fire_bullet(p_weapon_slot, p_character);
					next_possible_primary_attack += get_fire_rate();
				}
			} break;
		}
	}
}

bool WeaponFirearmInstance::has_ammo_in_clip(BaseCharacter *p_character) const {
	return p_character->get_ammo_in_weapon_clip(get_weapon_name()) > 0;
}

void WeaponFirearmInstance::primary_attack(int p_weapon_slot, const WeaponButtonState &p_button_state, BaseCharacter *p_character) {
	if (fire_duration == 0.0f && next_possible_primary_attack <= LaniakeaMainLoop::get_singleton()->get_physics_time()) {
		next_possible_primary_attack = LaniakeaMainLoop::get_singleton()->get_physics_time();
		// Post update will take care of it
		return;
	}
}

void WeaponFirearmInstance::do_view_kick(BaseCharacter *p_character) {
	if (PlayerCharacter *player = Object::cast_to<PlayerCharacter>(p_character); player != nullptr) {
#define MAX_VERTICAL_KICK 2.0f //Degrees
#define SLIDE_LIMIT 1.0f //Seconds
		player->add_camera_kick(Math::deg_to_rad(MAX_VERTICAL_KICK), fire_duration, SLIDE_LIMIT);
	}
}

float WeaponFirearmInstance::get_max_distance() const {
	return 1000.0f;
}

bool WeaponFirearmInstance::uses_occluded_crosshair() const {
	return true;
}

float WeaponFirearmInstance::get_damage() const {
	return LaniakeaMainLoop::get_singleton()->get_game_rules()->get_ammo_type_damage(get_ammo_type());
}

int WeaponFirearmInstance::get_clip_capacity() const {
	return 10;
}

void WeaponFirearmInstance::fire_bullet(int p_weapon_slot, BaseCharacter *p_character) {
	Vector3 origin;
	Vector3 normal;
	get_aim_trajectory(p_weapon_slot, p_character, origin, normal);
	p_character->fire_bullet(origin, normal, get_max_distance(), get_ammo_type(), get_damage());
	do_view_kick(p_character);
	p_character->subtract_ammo_for_weapon(p_weapon_slot, this, 1);
}
