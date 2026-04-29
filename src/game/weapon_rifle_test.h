#pragma once

#include "weapon_firearm.h"

class WeaponRifleTest : public WeaponFirearmInstance {
    GDCLASS(WeaponRifleTest, WeaponFirearmInstance);
    static void _bind_methods() {}
    virtual FireMode get_fire_mode() const override {
        return FireMode::FULL_AUTO;
    }
    virtual float get_fire_rate() const override {
        return 0.075f;
    }

    virtual BipedAnimationBase::WeaponAnimationType get_weapon_animation_type() const override { return BipedAnimationBase::WEAPON_ANIMATION_TYPE_RIFLE; };
    virtual WeaponModel *instantiate_visuals() const override;
public:
    virtual StringName get_item_name() const override;
};