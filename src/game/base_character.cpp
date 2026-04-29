#include "base_character.h"
#include "debug/debug_overlay.h"
#include "game/biped_animation_base.h"
#include "game/bullet_trail.h"
#include "game/character_hitbox_detector.h"
#include "game/damageable.h"
#include "game/main_loop.h"
#include "game/movement_settings.h"
#include "game/weapon_model.h"
#include "godot_cpp/classes/node.hpp"
#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/classes/physics_direct_space_state3d.hpp"
#include "godot_cpp/classes/physics_ray_query_parameters3d.hpp"
#include "godot_cpp/classes/physics_server3d.hpp"
#include "godot_cpp/classes/scene_tree.hpp"
#include "godot_cpp/classes/window.hpp"
#include "godot_cpp/classes/world3d.hpp"
#include "godot_cpp/core/print_string.hpp"
#include "physics_layers.h"
#include "game/weapon_instance.h"

void BaseCharacter::_bind_methods() {
    MAKE_BIND_NODE(BaseCharacter, model, CharacterModel);
}

Movement::MovementSpeed BaseCharacter::get_desired_movement_speed() const {
    static StringName move_sprint = "sprint";

    const Vector2 movement_vector = get_movement_vector();
    const float movement_vector_length = movement_vector.length();
    if (is_action_pressed(InputCommand::SPRINT)) {
        return movement_vector_length > 0.0f ? Movement::MovementSpeed::SPRINTING : Movement::MovementSpeed::IDLING;
    }

    if (movement_vector_length > 0.5f) {
        return Movement::MovementSpeed::RUNNING;
    }
    if (movement_vector_length > 0.0f) {
        return Movement::MovementSpeed::WALKING;
    }

    return Movement::MovementSpeed::IDLING;
}

bool BaseCharacter::is_action_pressed(InputCommand p_command) const {
    return get_action_state(p_command).has_flag(InputActionState::PRESSED);
}

bool BaseCharacter::is_action_just_pressed(InputCommand p_command) const {
    return get_action_state(p_command).has_flag(InputActionState::JUST_PRESSED);
}

bool BaseCharacter::is_action_just_released(InputCommand p_command) const {
    return get_action_state(p_command).has_flag(InputActionState::JUST_RELEASED);
}

void BaseCharacter::get_aim_trajectory(int p_wapon_slot, Vector3 &r_origin, Vector3 &r_direction) {
    r_origin = Vector3();
    r_direction = Vector3();
}

void BaseCharacter::fire_bullet(const Vector3 &p_origin, const Vector3 &p_direction, float p_distance, int p_ammo_type, float p_damage, bool p_fire_visual_from_origin) {
    //DebugOverlay::line(p_origin, p_origin + p_direction * p_distance, Color(1.0, 0.0, 0.0), false, 5.0f);
    Ref<PhysicsRayQueryParameters3D> params;
    params.instantiate();
    params->set_from(p_origin);
    params->set_to(p_origin + p_direction * p_distance);
    params->set_collision_mask(PhysicsLayers::LAYER_WORLDSPAWN | PhysicsLayers::LAYER_ENTITY_HITBOXES);
    params->set_exclude(attack_collision_exceptions);

    PhysicsDirectSpaceState3D *dss = get_world_3d()->get_direct_space_state();
    Dictionary result = dss->intersect_ray(params);

    const Vector3 firing_end = result.is_empty() ? params->get_to() : Vector3(result["position"]);

    DebugOverlay::sphere(firing_end, 0.25f, Color(1.0f, 0.0f, 0.0f));

    if (!result.is_empty()) {
        Node *n = Object::cast_to<Node>(result["collider"]);
        if (n != nullptr) {
            DebugOverlay::text(result["position"], vformat("%s", n->get_name()), Color(0.0, 1.0, 0.0), true, 2.0f);
        }
    } else {
        print_line("Hit... nothing");
    }

    // Add trail

    BulletTrail *trail = memnew(BulletTrail);
    LaniakeaMainLoop::get_singleton()->get_root()->add_child(trail);

    Vector3 trail_origin = p_origin;

    if (!p_fire_visual_from_origin && per_slot_weapon_visual[WEAPON_SLOT_PRIMARY] != nullptr && per_slot_weapon_visual[WEAPON_SLOT_PRIMARY]->get_muzzle_location()) {
        trail_origin = per_slot_weapon_visual[WEAPON_SLOT_PRIMARY]->get_muzzle_location()->get_global_position();
    }

    trail->initialize(trail_origin, firing_end, 100.0f);

    if (result.is_empty()) {
        return;
    }

    if (IDamageable *as_damageable = dynamic_cast<IDamageable*>(result["collider"].get_validated_object()); as_damageable != nullptr) {
        as_damageable->on_bullet_damage_received(p_ammo_type, p_damage, result["position"], result["normal"], result["shape"]);
    }
}

void BaseCharacter::_physics_process(double p_delta) {
    if (Engine::get_singleton()->is_editor_hint()) {
        return;
    }

    if (animation != nullptr) {
        animation->physics_update(this, p_delta);
    }
    
    movement.set_desired_movement_speed(get_desired_movement_speed());
    movement.set_input_vector(get_movement_vector_transformed());
    movement.update(p_delta);

    for (int slot_i = 0; slot_i < WEAPON_SLOT_MAX; slot_i++) {
        Ref<WeaponInstanceBase> equipped_weapon = equipped_weapons[slot_i];
        const InputCommand input_command = slot_i == WEAPON_SLOT_PRIMARY ? InputCommand::PRIMARY_FIRE : InputCommand::SECONDARY_FIRE;
        const bool primary_pressed = is_action_pressed(input_command);
        if (equipped_weapon.is_valid()) {
            WeaponInstanceBase::WeaponButtonState button_state = {
                .fire = primary_pressed
            };

            if (primary_pressed) {
                equipped_weapon->primary_attack(slot_i, button_state, this);
            }
            equipped_weapon->post_update(slot_i, this, button_state);
        }
    }

    if (model) {
        model->update(p_delta);
    }
}

void BaseCharacter::_process(double p_delta) {
    if (Engine::get_singleton()->is_editor_hint()) {
        return;
    }
    animation->update(get_desired_movement_speed(), p_delta);
}

void BaseCharacter::_ready() {
    if (Engine::get_singleton()->is_editor_hint()) {
        return;
    }

    movement_settings = get_movement_settings();
    movement.initialize(movement_settings, this);
    animation = create_animation();
    if (animation != nullptr) {
        animation->initialize(movement_settings, model);
    }

    add_attack_collision_exception(get_hitbox_detector_body_rid());

    add_to_group("characters");
}

void BaseCharacter::add_attack_collision_exception(RID p_rid) {
    if (!p_rid.is_valid()) {
        return;
    }
    attack_collision_exceptions.push_back(p_rid);
}

void BaseCharacter::remove_attack_collision_exception(RID p_rid) {
    attack_collision_exceptions.erase(p_rid);
}

Ref<MovementSettings> BaseCharacter::get_movement_settings() const {
    Ref<MovementSettings> setts;
    setts.instantiate();
    return setts;
}

BaseCharacter::FacingDirectionMode BaseCharacter::get_facing_direction_mode() const {
    return facing_direction_mode;
}

void BaseCharacter::set_facing_direction_mode(const FacingDirectionMode &facing_direction_mode_)
{
    facing_direction_mode = facing_direction_mode_;
}

Vector3 BaseCharacter::get_firing_position(int p_weapon_slot) const {
    if (model != nullptr) {
        return model->get_firing_position_node()->get_global_position();
    }
    return Vector3();
}

Ref<WeaponInstanceBase> BaseCharacter::get_equipped_weapon(const WeaponSlot p_slot) const {
    ERR_FAIL_INDEX_V(p_slot, WEAPON_SLOT_MAX, nullptr);
    return equipped_weapons[p_slot];
}

Vector3 BaseCharacter::get_facing_direction() const {
    return model->get_target_facing_direction();
}

void BaseCharacter::add_collision_exception(RID p_body) {
    movement.add_collision_exception(p_body);
}

void BaseCharacter::remove_collision_exception(RID p_body) {
    movement.remove_collision_exception(p_body);
}

RID BaseCharacter::get_hitbox_detector_body_rid() const {
    CharacterHitboxDetector *hitbox_detector = get_model()->get_hitbox_detector();
    if (hitbox_detector) {
        return hitbox_detector->get_rid();
    }

    return RID();
}

BaseCharacter::BaseCharacter() {
    set_physics_interpolation_mode(PHYSICS_INTERPOLATION_MODE_ON);
}

BaseCharacter::~BaseCharacter() {
    if (animation != nullptr) {
        memdelete(animation);
    }
}

void BaseCharacter::equip_weapon(WeaponSlot p_slot, Ref<WeaponInstanceBase> p_weapon) {
    if (equipped_weapons[p_slot].is_valid()) {
        if (per_slot_weapon_visual[p_slot] != nullptr) {
            per_slot_weapon_visual[p_slot]->queue_free();
            per_slot_weapon_visual[p_slot] = nullptr;
        }
        equipped_weapons[p_slot]->unequipped(p_slot, this);
        equipped_weapons[p_slot] = Ref<WeaponInstanceBase>();
    }

    equipped_weapons[p_slot] = p_weapon;
    
    if (!p_weapon.is_valid()) {
        return;
    }

    p_weapon->equipped(p_slot, this);

    if (model->get_hand_attachment_node() == nullptr) {
        return;
    }

    if (WeaponModel *weapon_visual = p_weapon->instantiate_visuals(); weapon_visual != nullptr) {
        model->get_hand_attachment_node()->add_child(weapon_visual);
        per_slot_weapon_visual[p_slot] = weapon_visual;
    }
}