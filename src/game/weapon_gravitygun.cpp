#include "weapon_gravitygun.h"
#include "console/cvar.h"
#include "debug/debug_overlay.h"
#include "game/main_loop.h"
#include "game/physics_prop.h"
#include "game/player_character.h"
#include "gdextension_interface.h"
#include "base_character.h"
#include "godot_cpp/classes/generic6_dof_joint3d.hpp"
#include "godot_cpp/classes/physics_direct_space_state3d.hpp"
#include "godot_cpp/classes/physics_ray_query_parameters3d.hpp"
#include "godot_cpp/classes/physics_server3d.hpp"
#include "godot_cpp/classes/scene_tree_timer.hpp"
#include "godot_cpp/classes/static_body3d.hpp"
#include "godot_cpp/classes/world3d.hpp"
#include "godot_cpp/variant/dictionary.hpp"
#include "godot_cpp/classes/window.hpp"
#include "physics_layers.h"

CVar WeaponGravityGun::pull_force_cvar = CVar::create_variable("gravity_gun_pull_force", GDEXTENSION_VARIANT_TYPE_FLOAT, 100.0f, "Force to pull objects towards the player when they are not yet grabbable");
CVar WeaponGravityGun::pull_threshold_cvar = CVar::create_variable("gravity_gun_pull_threshold", GDEXTENSION_VARIANT_TYPE_FLOAT, 15.0f, "Maximum distance to the object to be slowly pulled towards the player.");
CVar WeaponGravityGun::grab_threshold_cvar = CVar::create_variable("gravity_gun_grab_threshold", GDEXTENSION_VARIANT_TYPE_FLOAT, 5.0f, "Maximum distance to the object to be considered grabbable");
CVar WeaponGravityGun::grabbed_offset_cvar = CVar::create_variable("gravity_gun_grabbed_offset", GDEXTENSION_VARIANT_TYPE_VECTOR3, Vector3(0.0f, 2.5f, 0.0f), "Offset from the player for grabbed objects");
CVar WeaponGravityGun::throw_velocity_cvar = CVar::create_variable("gravity_gun_throw_velocity", GDEXTENSION_VARIANT_TYPE_FLOAT, 30.0f, "Force with which to throw grabbed objects");
CVar WeaponGravityGun::throw_angular_velocity_max_cvar = CVar::create_variable("gravity_gun_throw_angular_velocity_max", GDEXTENSION_VARIANT_TYPE_FLOAT, (float)Math_PI*3.0f, "Random angular velocity to add to a thrown object");

Vector3 WeaponGravityGun::_random_angular_velocity(float p_min_rads, float p_max_rads) const {
	return Vector3(
        UtilityFunctions::randf_range(p_min_rads, p_max_rads),
        UtilityFunctions::randf_range(p_min_rads, p_max_rads),
        UtilityFunctions::randf_range(p_min_rads, p_max_rads)
    );
}

void WeaponGravityGun::primary_attack(int p_weapon_slot, const WeaponButtonState &p_button_state, BaseCharacter *p_character) {
    const bool just_pressed_attack = p_character->is_action_just_pressed(BaseCharacter::SECONDARY_FIRE);
    if (needs_attack_repress && just_pressed_attack) {
        needs_attack_repress = false;
    }

    if (needs_attack_repress) {
        return;
    }

    if (attached_prop.has_value() && just_pressed_attack) {
        do_throw(p_character);
        needs_attack_repress = true;
        return;
    }

    if (attached_prop.has_value()) {
        return;
    }

    // Find object in front of us
    Vector3 aim_origin;
    Vector3 aim_normal;
    p_character->get_aim_trajectory(p_weapon_slot, aim_origin, aim_normal);

    DebugOverlay::line(aim_origin, aim_origin + aim_normal, Color::named("WEB_PURPLE"));

    Ref<PhysicsRayQueryParameters3D> params;
    params.instantiate();
    params->set_from(aim_origin);
    params->set_to(aim_origin + aim_normal * pull_threshold_cvar.get_float());
    params->set_collision_mask(PhysicsLayers::LAYER_PROPS | PhysicsLayers::LAYER_WORLDSPAWN | PhysicsLayers::LAYER_ENTITY_HITBOXES);

    PhysicsDirectSpaceState3D *dss = p_character->get_world_3d()->get_direct_space_state();
    Dictionary ray_out = dss->intersect_ray(params);
    DebugOverlay::line(params->get_from(), params->get_to(), Color::named("VIOLET"));

    if (ray_out.is_empty()) {
        return;
    }

    Node *collider = Object::cast_to<Node>(ray_out["collider"]);
    LaniakeaPhysicsProp *prop = Object::cast_to<LaniakeaPhysicsProp>(ray_out["collider"]);
    if (prop == nullptr) {
        // We hit something, but it wasn't something we could grab, oh well.
        return;
    }

    // If the object is too far, we will just push it towards us, otherwise we will grab it

    const Vector3 prop_pos = prop->get_global_position();
    if (prop_pos.distance_to(p_character->get_model()->get_global_position()) <= grab_threshold_cvar.get_float()) {
        grab_object(prop, p_character);
        return;
    }

    // Push object towards us
    prop->apply_central_force(prop->get_global_position().direction_to(p_character->get_model()->get_global_position()) * 550.0f);
}

void WeaponGravityGun::post_update(int p_weapon_slot, BaseCharacter *p_character, const WeaponButtonState &p_button_state) {
    if (attached_prop.has_value()) {
        attached_prop->attachment_body->set_global_basis(Basis::looking_at(p_character->get_facing_direction()));
        attached_prop->attachment_body->set_global_position(grabbed_offset_cvar.get_vector3() + p_character->get_model()->get_global_position());

        DebugOverlay::sphere(attached_prop->attachment_body->get_global_position(), 0.1f, Color("WEB_PURPLE"));
    }
}

Generic6DOFJoint3D *WeaponGravityGun::_create_joint() const {
    Generic6DOFJoint3D *joint = memnew(Generic6DOFJoint3D);

    const float stiffness = 3000.0f;
    const float damping = 100.0f;

    joint->set_param_x(Generic6DOFJoint3D::PARAM_LINEAR_SPRING_STIFFNESS, stiffness);
    joint->set_param_x(Generic6DOFJoint3D::PARAM_LINEAR_SPRING_DAMPING, damping);
    joint->set_param_y(Generic6DOFJoint3D::PARAM_LINEAR_SPRING_STIFFNESS, stiffness);
    joint->set_param_y(Generic6DOFJoint3D::PARAM_LINEAR_SPRING_DAMPING, damping);
    joint->set_param_z(Generic6DOFJoint3D::PARAM_LINEAR_SPRING_STIFFNESS, stiffness);
    joint->set_param_z(Generic6DOFJoint3D::PARAM_LINEAR_SPRING_DAMPING, damping);

    const bool use_linear_spring = true;
    const bool use_linear_limit = false;

    joint->set_flag_x(Generic6DOFJoint3D::FLAG_ENABLE_LINEAR_SPRING, use_linear_spring);
    joint->set_flag_x(Generic6DOFJoint3D::FLAG_ENABLE_LINEAR_LIMIT, use_linear_limit);
    joint->set_flag_y(Generic6DOFJoint3D::FLAG_ENABLE_LINEAR_SPRING, use_linear_spring);
    joint->set_flag_y(Generic6DOFJoint3D::FLAG_ENABLE_LINEAR_LIMIT, use_linear_limit);
    joint->set_flag_z(Generic6DOFJoint3D::FLAG_ENABLE_LINEAR_SPRING, use_linear_spring);
    joint->set_flag_z(Generic6DOFJoint3D::FLAG_ENABLE_LINEAR_LIMIT, use_linear_limit);

    return joint;
}

void WeaponGravityGun::grab_object(LaniakeaPhysicsProp *p_prop, BaseCharacter *p_character) {
    attached_prop = AttachedProp {
        .original_mass = p_prop->get_mass(),
        .could_sleep = PhysicsServer3D::get_singleton()->body_get_state(p_prop->get_rid(), PhysicsServer3D::BODY_STATE_CAN_SLEEP),
        .prop_object = ObjectID(p_prop->get_instance_id()),
        .attachment_body = memnew(StaticBody3D),
        .attachment_joint = _create_joint()
    };

    Node *root = LaniakeaMainLoop::get_singleton()->get_root();
    root->add_child(attached_prop->attachment_body);
    root->add_child(attached_prop->attachment_joint);

    attached_prop->attachment_joint->set_position(p_prop->get_global_position());
    attached_prop->attachment_body->set_position(p_prop->get_global_position());
    attached_prop->attachment_body->set_basis(Basis::looking_at(p_character->get_facing_direction()));
    attached_prop->attachment_joint->set_node_b(attached_prop->attachment_body->get_path());
    attached_prop->attachment_joint->set_node_a(p_prop->get_path());
    
    p_prop->set_use_continuous_collision_detection(true);
    p_prop->set_can_sleep(false);
    p_prop->set_mass(1.0f);
    p_character->add_collision_exception(p_prop->get_rid());

    if (PlayerCharacter *player = Object::cast_to<PlayerCharacter>(p_character); player != nullptr) {
        player->add_aim_occlusion_exception(p_prop->get_rid());
    }
}

void WeaponGravityGun::drop_current_object() {
    DEV_ASSERT(attached_prop.has_value());

    // Remove joints
    attached_prop->attachment_joint->queue_free();
    attached_prop->attachment_body->queue_free();

    LaniakeaPhysicsProp *prop = Object::cast_to<LaniakeaPhysicsProp>(ObjectDB::get_instance(attached_prop->prop_object));

    if (!prop) {
        return;
    }

    prop->set_mass(attached_prop->original_mass);
    prop->set_use_continuous_collision_detection(false);
    prop->set_can_sleep(attached_prop->could_sleep);
    attached_prop.reset();
}

void WeaponGravityGun::do_throw(BaseCharacter *p_character) {
    LaniakeaPhysicsProp *prop = Object::cast_to<LaniakeaPhysicsProp>(ObjectDB::get_instance(attached_prop->prop_object));
    if (!prop) {
        drop_current_object();
        return;
    }

    drop_current_object();


    PlayerCharacter *player = Object::cast_to<PlayerCharacter>(p_character);
    if (player == nullptr) {
        return;
    }

    Vector3 target = player->get_slot_target_position(BaseCharacter::WEAPON_SLOT_SECONDARY);
    DebugOverlay::sphere(target, 0.1f, Color("HOTPINK"), false, 5.0f);
    prop->set_linear_velocity(prop->get_linear_velocity() + prop->get_global_position().direction_to(target) * throw_velocity_cvar.get_float());
    prop->set_angular_velocity(prop->get_angular_velocity() + _random_angular_velocity(-throw_angular_velocity_max_cvar.get_float(), throw_angular_velocity_max_cvar.get_float()));

    Ref<SceneTreeTimer> self_collision_grace_period_timer = LaniakeaMainLoop::get_singleton()->create_timer(0.1, false, true);
    self_collision_grace_period_timer->connect("timeout", callable_mp(p_character, &BaseCharacter::remove_collision_exception).bind(prop->get_rid()));
}

float WeaponGravityGun::get_max_distance() const {
    return pull_threshold_cvar.get_float();
}

StringName WeaponGravityGun::get_item_name() const {
    static StringName s = "weapon_gravitygun_item";
    return s;
}

