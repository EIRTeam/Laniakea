#pragma once

#include "game/base_character.h"
#include "game/weapon_instance.h"

class WeaponFirearmInstance : public WeaponInstanceBase {
    GDCLASS(WeaponFirearmInstance, WeaponInstanceBase);

protected:
    float fire_duration = 0.0f;
    float next_possible_primary_attack = 0.0f;
    float reload_finish_time = 0.0f;
    bool reloading = false;
public:
    enum FireMode {
        SEMI_AUTO,
        FULL_AUTO
    };
    static void _bind_methods();
    bool player_is_reloading() const;
    virtual void get_aim_trajectory(int p_weapon_slot, BaseCharacter *p_character, Vector3 &r_origin, Vector3 &r_direction) const;
    virtual void post_update(int p_weapon_slot, BaseCharacter *p_character, const WeaponButtonState &p_button_state) override;
    bool has_ammo_in_clip(BaseCharacter *p_character) const;
    virtual void primary_attack(int p_weapon_slot, const WeaponButtonState &p_button_state, BaseCharacter *p_character) override;
    virtual void do_view_kick(BaseCharacter *p_character);
    // In rounds per second please!
    virtual float get_fire_rate() const = 0;
    virtual float get_max_distance() const override;
    virtual FireMode get_fire_mode() const { return FireMode::SEMI_AUTO; }
    virtual bool uses_occluded_crosshair() const override;
    virtual int get_ammo_type() const = 0;
    virtual float get_damage() const;
    virtual int get_clip_capacity() const;
    void fire_bullet(int p_weapon_slot, BaseCharacter *p_character);
};