#pragma once

#include "game_rules.h"

class LaniakeaGameRules : public GameRules {
    GDCLASS(LaniakeaGameRules, GameRules);
public:
    enum AmmoTypes {
        RIFLE_AMMO_TYPE,
        AMMO_TYPE_MAX 
    };
private:
    // Rifle ammo
    static CVar rifle_max_ammo_cvar;
    static CVar rifle_player_damage_cvar;
    static CVar rifle_npc_damage_cvar;

    static void _bind_methods() {}
public:
    virtual void initialize() override;
};