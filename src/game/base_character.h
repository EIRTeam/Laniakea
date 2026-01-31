#pragma once

#include "game/base_movement.h"
#include "game/biped_animation_base.h"
#include "game/character_animation_base.h"
#include "game/character_model.h"
#include "godot_cpp/core/binder_common.hpp"
#include "godot_cpp/core/error_macros.hpp"

using namespace godot;

class WeaponModel;
class WeaponInstanceBase;

class BaseCharacter : public Node3D {
    GDCLASS(BaseCharacter, Node3D);
protected:
    BaseMovement movement;
    CharacterAnimationBase* animation = nullptr;
public:
    enum WeaponSlot {
        WEAPON_SLOT_PRIMARY,
        WEAPON_SLOT_SECONDARY,
        WEAPON_SLOT_MAX
    };
protected:
    std::array<WeaponModel*, WEAPON_SLOT_MAX> per_slot_weapon_visual = { nullptr };
    std::array<Ref<WeaponInstanceBase>, WEAPON_SLOT_MAX> equipped_weapons;
    Ref<MovementSettings> movement_settings;
    CharacterModel *model = nullptr;
    static void _bind_methods();
public:
    MAKE_SETTER_GETTER_VALUE(CharacterModel *, model, model);
    enum InputState {
        PRESSED = 1,
        JUST_PRESSED = 2,
        JUST_RELEASED = 4
    };

    enum InputCommand {
        PRIMARY_FIRE,
        SECONDARY_FIRE,
        AIM,
        SPRINT,
        INPUT_COMMAND_MAX
    };
    struct CharacterInputState {
        Vector2 movement_input;
        std::array<int, INPUT_COMMAND_MAX> button_states;
    };

    CharacterInputState input_state;

    enum FacingDirectionMode {
        TO_MOVEMENT,
        CUSTOM
    };

    FacingDirectionMode facing_direction_mode = TO_MOVEMENT;

    Movement::MovementSpeed get_desired_movement_speed() const;
    Vector2 get_input_vector() const;
    virtual Vector2 get_input_vector_transformed() const;
    bool is_action_pressed(InputCommand p_command) const;
    virtual void get_aim_trajectory(int p_weapon_slot, Vector3 &r_origin, Vector3 &r_direction);
    virtual void fire_bullet(const Vector3 &p_origin, const Vector3 &p_direction, float p_distance, int p_ammo_type, float p_damage);
    void set_input_state(const CharacterInputState &p_input_state);
    CharacterInputState get_input_state() const;
    virtual void _physics_process(double p_delta) override;
    virtual void _process(double p_delta) override;
    virtual void _ready() override;
    virtual Ref<MovementSettings> get_movement_settings() const;
    BaseCharacter();
    virtual ~BaseCharacter();

    FacingDirectionMode get_facing_direction_mode() const;
    void set_facing_direction_mode(const FacingDirectionMode &facing_direction_mode_);
    virtual Vector3 get_firing_position(int p_weapon_slot) const;
    void equip_weapon(WeaponSlot p_slot, Ref<WeaponInstanceBase> p_weapon);
    Ref<WeaponInstanceBase> get_equipped_weapon(const WeaponSlot p_slot) const;
    Vector3 get_facing_direction() const;
    void add_collision_exception(RID p_body);
    void remove_collision_exception(RID p_body);

    virtual Vector<StringName> get_available_items(WeaponSlot p_slot) const { return Vector<StringName>(); }
    virtual CharacterAnimationBase *create_animation() const = 0;

    BaseMovement *get_movement() { return &movement; };
};

VARIANT_ENUM_CAST(BaseCharacter::WeaponSlot)