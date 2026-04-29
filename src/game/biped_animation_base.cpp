#include "biped_animation_base.h"
#include "game/character_model.h"
#include "game/movement_shared.h"
#include "godot_cpp/classes/animation_mixer.hpp"
#include "godot_cpp/classes/animation_node_blend_tree.hpp"
#include "godot_cpp/classes/animation_node_state_machine.hpp"
#include "godot_cpp/classes/animation_node_state_machine_playback.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "animation/inertialization_skeleton_modifier_polynomial.h"
#include "animation/hip_rotator_modifier.h"
#include "base_movement.h"
#include "base_character.h"



StringName BipedAnimationBase::get_animation_state_string_name(const AnimationState p_state) const {
    switch (p_state) {
        case AnimationState::LOCOMOTION: {
            return "Locomotion";
        };
        case AnimationState::MILK_HANGING: {
            return "MilkHanging";
        }
        case AnimationState::RUN_TO_STOP: {
            return "RunToStop";
        };
    }
}

String BipedAnimationBase::get_upper_body_animation_state_string(const UpperBodyAnimationState p_state) const {
    switch (p_state) {
		case RIFLE_IDLE: {
            return "RifleIdle";
        } break;
		case RIFLE_AIM: {
            return "RifleAim";
        } break;
	}
}

void BipedAnimationBase::_bind_methods() {}

Ref<AnimationNodeStateMachinePlayback> BipedAnimationBase::get_state_machine() const {
    const Ref<AnimationNodeStateMachinePlayback> playback = model->get_animation_tree()->get("parameters/playback");
    DEV_ASSERT(playback.is_valid());
    return playback;
}

StringName BipedAnimationBase::get_current_locomotion_blend_space_name() const {
    switch(locomotion_state) {
		case STANDING_STRAFING: {
            return "StandingStrafing";
        } break;
	}
    return "";
}

void BipedAnimationBase::_update_locomotion_blend_state() {
    const StringName blend_space_node_name = get_current_locomotion_blend_space_name();
    if (blend_space_node_name.is_empty()) {
        return;
    }

    AnimationTree *animation_tree = model->get_animation_tree();

    static StringName LOCOMOTION_TIME_SCALE_PROPERTY_NAME = "parameters/Locomotion/LocomotionTimeScale/scale";
    
    animation_tree->set(LOCOMOTION_TIME_SCALE_PROPERTY_NAME, 1.0f);
    
    const float velocity = locomotion_effective_velocity.length();
    float animation_velocity = velocity;

    const float MAX_WALK_SPEED = movement_settings->get_stance_max_walk_speed(movement_stance);
    
    animation_tree->set(LOCOMOTION_TIME_SCALE_PROPERTY_NAME, per_stance_movement_animation_speed_curve[movement_stance]->sample(velocity));
    
    
    const StringName blend_position_property_name = vformat("parameters/Locomotion/%s/blend_position", blend_space_node_name);
    const Vector2 out = Vector2(locomotion_angle, per_stance_movement_animation_blend_curve[movement_stance]->sample(animation_velocity));
    animation_tree->set(blend_position_property_name, out);
}

void BipedAnimationBase::set_locomotion_state(LocomotionAnimationState p_locomotion_state) {
    locomotion_state = p_locomotion_state;

    if (model == nullptr) {
        return;
    }
}

BipedAnimationBase::LocomotionAnimationState BipedAnimationBase::get_locomotion_state() const {
    return locomotion_state;
}

void BipedAnimationBase::initialize(Ref<MovementSettings> p_movement_settings, CharacterModel *p_model) {
    movement_settings = p_movement_settings;
    model = p_model;
    DEV_ASSERT(p_model != nullptr);
    p_model->get_animation_tree()->set_callback_mode_process(AnimationMixer::ANIMATION_CALLBACK_MODE_PROCESS_MANUAL);
    p_model->get_animation_tree()->set_callback_mode_method(AnimationMixer::ANIMATION_CALLBACK_MODE_METHOD_IMMEDIATE);

    for (int i = 0; i < Movement::STANCE_MAX; i++) {
        Ref<Curve> curve;
        curve.instantiate();

        curve->set_max_value(200.0f);
        curve->set_max_domain(200.0f);

        Ref<Curve> blend_curve;
        blend_curve.instantiate();

        blend_curve->set_max_value(200.0f);
        blend_curve->set_max_domain(200.0f);


        // X: Velocity
        // Y: Animation speed scale
        
        // Idle
        curve->add_point(Vector2(0.0f, 1.0f));
        blend_curve->add_point(Vector2(0.0f, 0.0f));

        // Walk
        curve->add_point(Vector2(p_movement_settings->get_stance_max_walk_speed(i), 1.0f));
        blend_curve->add_point(Vector2(p_movement_settings->get_stance_max_walk_speed(i), 1.0f));

        // Run
        curve->add_point(Vector2(p_movement_settings->get_stance_max_run_speed(i), 2.0f));
        blend_curve->add_point(Vector2(p_movement_settings->get_stance_max_run_speed(i), 2.0f));
        
        // Sprint
        curve->add_point(Vector2(p_movement_settings->get_stance_max_sprint_speed(i), 3.0f));
        blend_curve->add_point(Vector2(p_movement_settings->get_stance_max_sprint_speed(i), 2.0f));

        per_stance_movement_animation_speed_curve[i] = curve;
        per_stance_movement_animation_blend_curve[i] = blend_curve;
    }
}

void BipedAnimationBase::update(Movement::MovementSpeed p_desired_movement_speed, float p_delta) {
    AnimationState current_state = get_current_animation_state();
    
    bool was_running = locomotion_effective_velocity.length() > movement_settings->get_standing_max_walk_speed();
    
    if (current_state == LOCOMOTION) {
        if (p_desired_movement_speed == Movement::IDLING && was_running) {
            //desired_animation_state = AnimationState::RUN_TO_STOP;
        } else if (desired_animation_state == RUN_TO_STOP) {
            desired_animation_state = LOCOMOTION;
        }
    } else if (current_state == RUN_TO_STOP) {
        if (p_desired_movement_speed != Movement::IDLING) {
            desired_animation_state = LOCOMOTION;
        }
    }

    if (is_hanging) {
        desired_animation_state = MILK_HANGING;
    }
    
    if (current_state != desired_animation_state) {
        get_state_machine()->travel(get_animation_state_string_name(desired_animation_state));
    }
    _update_locomotion_blend_state();

    _update_upper_body_state();

    if (previous_animation_state != get_state_machine()->get_current_node() || needs_inertialization) {
        InertializationSkeletonModifierPolynomial *inert = Object::cast_to<InertializationSkeletonModifierPolynomial>(model->get_skeleton()->find_child("InertializationSkeletonModifierPolynomial"));
        inert->queue_inertialization();
        needs_inertialization = false;
    }

    prev_desired_movement_speed = p_desired_movement_speed;
    previous_animation_state = get_state_machine()->get_current_node();
    model->get_animation_tree()->advance(p_delta);
}

void BipedAnimationBase::physics_update(BaseCharacter *p_character, float p_delta) {
    const Vector3 effective_vel = p_character->get_movement()->get_effective_velocity();
    set_locomotion_effective_velocity(effective_vel);

    if (effective_vel.length() > 0.1f) {
        const Vector3 movement_direction = effective_vel.normalized();
        const float angle = Math::rad_to_deg(movement_direction.signed_angle_to(model->get_target_facing_direction(), Vector3(0.0f, 1.0f, 0.0f)));
        set_locomotion_angle(angle);
    }
}

BipedAnimationBase::UpperBodyAnimationState BipedAnimationBase::get_current_upper_body_animation_state() const {
    const static StringName rifle_aim_state = StringName("RifleAim");
    const static StringName rifle_idle_state = StringName("RifleIdle");

    const StringName curr_state = model->get_animation_tree()->get("parameters/Locomotion/UpperBodyAnimationState/current_state");

    if (curr_state == rifle_aim_state) {
        return UpperBodyAnimationState::RIFLE_AIM;
    } else if (curr_state == rifle_idle_state) {
        return UpperBodyAnimationState::RIFLE_IDLE;
    }

    ERR_FAIL_V_MSG(RIFLE_IDLE, "Unknown state!?");
}

void BipedAnimationBase::set_upper_body_animation_state(const UpperBodyAnimationState p_state) const {
    model->get_animation_tree()->set("parameters/Locomotion/UpperBodyAnimationState/transition_request", get_upper_body_animation_state_string(p_state));
}

BipedAnimationBase::AnimationState BipedAnimationBase::get_current_animation_state() const {
    const static StringName locomotion_state = StringName("Locomotion");
    const static StringName run_to_stop_state = StringName("RunToStop");
    const static StringName milk_hanging = StringName("MilkHanging");
    
    const Ref<AnimationNodeStateMachinePlayback> playback = get_state_machine();
    const StringName current_state = playback->get_current_node();
    if (current_state == locomotion_state) {
        return AnimationState::LOCOMOTION;
    } else if (current_state == milk_hanging) {
        return AnimationState::MILK_HANGING;
    } else if (current_state == run_to_stop_state) {
        return AnimationState::RUN_TO_STOP;
    }

    ERR_FAIL_V_MSG(BipedAnimationBase::AnimationState::LOCOMOTION, "Unknown state!");
}

BipedAnimationBase::AnimationState BipedAnimationBase::get_desired_animation_state() const {
    return desired_animation_state;
}

void BipedAnimationBase::set_desired_animation_state(const AnimationState &p_animation_state) {
    desired_animation_state = p_animation_state;
}

BipedAnimationBase::WeaponAnimationType BipedAnimationBase::get_weapon_animation_type() const
{
    return weapon_animation_type;
}

void BipedAnimationBase::set_weapon_animation_type(const WeaponAnimationType &p_weapon_animation_type) {
    const bool changed = p_weapon_animation_type != weapon_animation_type;
    weapon_animation_type = p_weapon_animation_type;
    needs_inertialization = needs_inertialization || changed;
}

float BipedAnimationBase::get_aim_x_angle() const
{
    return aim_x_angle;
}

void BipedAnimationBase::set_aim_x_angle(float aim_x_angle_)
{
    aim_x_angle = aim_x_angle_;
}

void BipedAnimationBase::set_hanging(bool p_hanging) {
    is_hanging = p_hanging;
}

void BipedAnimationBase::_update_upper_body_blend() {

}

void BipedAnimationBase::_update_upper_body_state() {
    UpperBodyAnimationState state;

    float blend = 1.0f;
    float hip_rotator_influence = 0.0f;

    switch(weapon_animation_type) {
		case WEAPON_ANIMATION_TYPE_NONE: {
            state = UpperBodyAnimationState::RIFLE_IDLE;
            blend = 0.0f;
        } break;
		case WEAPON_ANIMATION_TYPE_RIFLE: {
            if (is_aiming) {
                hip_rotator_influence = 1.0f;
                blend = 1.0f;
                state = UpperBodyAnimationState::RIFLE_AIM;
            } else {
                blend = 0.0f;
                state = UpperBodyAnimationState::RIFLE_IDLE;
            }
        } break;
	}

    float current_blend = model->get_animation_tree()->get("parameters/Locomotion/UpperBodyBlend/blend_amount");

    if (model->get_hip_rotator() != nullptr) {
        model->get_hip_rotator()->set_target_aim_angle(aim_x_angle);
        if (model->get_hip_rotator()->get_influence() != hip_rotator_influence) {
            model->get_hip_rotator()->set_influence(hip_rotator_influence);
            needs_inertialization = true;
        }
    }


    if (state != get_current_upper_body_animation_state() || current_blend != blend) {
        model->get_animation_tree()->set("parameters/Locomotion/UpperBodyBlend/blend_amount", blend);
        set_upper_body_animation_state(state);
        needs_inertialization = true;
    }
}

bool BipedAnimationBase::get_is_aiming() const
{
    return is_aiming;
}

void BipedAnimationBase::set_is_aiming(bool p_is_aiming)
{
    is_aiming = p_is_aiming;
}
