#include "weapon_firearm.h"
#include "game/main_loop.h"
#include "game/player_character.h"
#include "game/protagonist_player_character.h"

void WeaponFirearmInstance::_bind_methods() {
    
}

void WeaponFirearmInstance::get_aim_trajectory(int p_weapon_slot, BaseCharacter *p_character, Vector3 &r_origin, Vector3 &r_direction) const {
    return p_character->get_aim_trajectory(p_weapon_slot, r_origin, r_direction);
}

void WeaponFirearmInstance::post_update(int p_weapon_slot, BaseCharacter *p_character, const WeaponButtonState &p_button_state) {
    fire_duration = p_button_state.fire ? fire_duration + p_character->get_physics_process_delta_time() : 0.0f;

    const float time = LaniakeaMainLoop::get_singleton()->get_physics_time();

    if (p_button_state.fire && next_possible_primary_attack <= time) {
        switch (get_fire_mode()) {
            case SEMI_AUTO: {
                Vector3 origin;
                Vector3 normal;
                get_aim_trajectory(p_weapon_slot, p_character, origin, normal);
                p_character->fire_bullet(origin, normal, get_max_distance(), 0, 20.0f);
                do_view_kick(p_character);
            } break;
            case FULL_AUTO: {
                while (p_button_state.fire && next_possible_primary_attack <= time) {
                    Vector3 origin;
                    Vector3 normal;
                    get_aim_trajectory(p_weapon_slot, p_character, origin, normal);
                    p_character->fire_bullet(origin, normal, get_max_distance(), 0, 20.0f);
                    next_possible_primary_attack += get_fire_rate();
                    do_view_kick(p_character);
                }
            } break;
        }
    }
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
        #define	MAX_VERTICAL_KICK	2.0f	//Degrees
        #define	SLIDE_LIMIT			1.0f	//Seconds
        player->add_camera_kick(Math::deg_to_rad(MAX_VERTICAL_KICK), fire_duration, SLIDE_LIMIT);
    }
}

float WeaponFirearmInstance::get_max_distance() const { return 1000.0f; };

bool WeaponFirearmInstance::uses_occluded_crosshair() const {
    return true;
}
