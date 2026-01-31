#pragma once

#include "game/character_model.h"
#include "game/milk_animation.h"
#include "godot_cpp/classes/flow_container.hpp"
#include "player_character.h"

class PlayerCharacterProtagonist : public PlayerCharacter {
    GDCLASS(PlayerCharacterProtagonist, PlayerCharacter);
    CharacterModel *milk_model = nullptr;
    MilkAnimation* milk_animation = nullptr;

    Basis prev_player_model_basis;
public:
    Vector3 prev_milk_pos_relative_to_player;
    static void _bind_methods();
    virtual void _ready() override;
    void attach_milk_model(CharacterModel *p_model);
    CharacterModel *get_milk_model() const;
    void _process(double p_delta) override;
    void _physics_process(double p_delta) override;
    virtual Vector3 get_firing_position(int p_weapon_slot) const override;
    
    ~PlayerCharacterProtagonist();
};