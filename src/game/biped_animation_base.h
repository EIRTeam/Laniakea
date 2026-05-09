#pragma once

#include "game/character_animation_base.h"
#include "game/movement_settings.h"
#include "game/movement_shared.h"
#include "godot_cpp/classes/animation_node_state_machine_playback.hpp"
#include "godot_cpp/classes/curve.hpp"
#include <array>
#include <optional>

class CharacterModel;

class BipedAnimationBase : public CharacterAnimationBase {
    GDCLASS(BipedAnimationBase, Object);
public:
    enum LocomotionAnimationState {
        STANDING_STRAFING
    };
    enum AnimationState {
        LOCOMOTION,
        MILK_HANGING,
        RUN_TO_STOP
    };

    enum UpperBodyAnimationState {
        RIFLE_IDLE,
        RIFLE_AIM,
        UPPER_BODY_STATE_RELOAD,
        UPPER_BODY_STATE_MAX
    };
protected:
    bool is_aiming = false;
    bool is_hanging = false;
    float aim_x_angle = 0.0f;
    Ref<MovementSettings> movement_settings;
    CharacterModel *model;
public:
    enum WeaponAnimationSetType {
        WEAPON_ANIMATION_TYPE_NONE,
        WEAPON_ANIMATION_TYPE_RIFLE,
        WEAPON_ANIMATION_SET_TYPE_MAX
    };
protected:
    WeaponAnimationSetType weapon_animation_set = WEAPON_ANIMATION_TYPE_NONE;

    AnimationState desired_animation_state = AnimationState::LOCOMOTION;
    LocomotionAnimationState locomotion_state = LocomotionAnimationState::STANDING_STRAFING;
    Movement::MovementStance movement_stance = Movement::STANDING ;
    Vector3 locomotion_effective_velocity;
    float locomotion_angle = 0.0f;
    Movement::MovementSpeed prev_desired_movement_speed = Movement::MovementSpeed::WALKING;

    StringName previous_animation_state;

    std::array<Ref<Curve>, Movement::MovementStance::STANCE_MAX> per_stance_movement_animation_speed_curve;
    std::array<Ref<Curve>, Movement::MovementStance::STANCE_MAX> per_stance_movement_animation_blend_curve;

    StringName get_animation_state_string_name(const AnimationState p_state) const;
    String get_upper_body_animation_state_string(const UpperBodyAnimationState p_state) const;

    struct CurrentUpperBodySequence {
        StringName node_path;
        UpperBodyAnimationState upper_body_state;
    };

    std::optional<CurrentUpperBodySequence> current_upper_body_sequence;

public:
    static void _bind_methods();
    Ref<AnimationNodeStateMachinePlayback> get_state_machine() const;
    StringName get_current_locomotion_blend_space_name() const;
    void _update_locomotion_blend_state();
    void set_locomotion_state(LocomotionAnimationState p_locomotion_state);
    LocomotionAnimationState get_locomotion_state() const;
    virtual void initialize(Ref<MovementSettings> p_movement_settings, CharacterModel *p_model) override;
    virtual void update(Movement::MovementSpeed p_desired_movement_speed, float p_delta) override;
    virtual void physics_update(BaseCharacter *p_character, float p_delta) override;

    Vector3 get_locomotion_effective_velocity() const { return locomotion_effective_velocity; }
    void set_locomotion_effective_velocity(const Vector3 &locomotionEffectiveVelocity) { locomotion_effective_velocity = locomotionEffectiveVelocity; }

    float get_locomotion_angle() const { return locomotion_angle; }
    void set_locomotion_angle(float locomotionAngle) { locomotion_angle = locomotionAngle; }

    UpperBodyAnimationState get_current_upper_body_animation_state() const;
    void set_upper_body_animation_state(const UpperBodyAnimationState p_state) const;
    AnimationState get_current_animation_state() const;
    AnimationState get_desired_animation_state() const;
    void set_desired_animation_state(const AnimationState &p_animation_state);
    void set_desired_upper_body_animation_state(const AnimationState &p_animation_state);

    void set_hanging(bool p_hanging);

    bool needs_inertialization = false;
    void _update_upper_body_blend();
    void _update_upper_body_state();

    WeaponAnimationSetType get_weapon_animation_set() const;
    void set_weapon_animation_set(const WeaponAnimationSetType &weapon_animation_type_);

    bool get_is_aiming() const;
    void set_is_aiming(bool is_aiming_);

    float get_aim_x_angle() const;
    void set_aim_x_angle(float aim_x_angle_);

    enum UpperBodySequence {
        SEQUENCE_RELOAD
    };

    void trigger_upper_body_sequence(UpperBodySequence p_sequence);
    double get_upper_body_sequence_duration(UpperBodySequence p_sequence) const;
    void abort_upper_body_sequence();
};