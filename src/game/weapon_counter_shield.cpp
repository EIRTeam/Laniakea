#include "weapon_counter_shield.h"
#include "debug/debug_overlay.h"
#include "game/base_character.h"
#include "game/player_character.h"
#include "gdextension_interface.h"
#include "godot_cpp/classes/base_material3d.hpp"
#include "godot_cpp/classes/mesh_instance3d.hpp"
#include "godot_cpp/classes/method_tweener.hpp"
#include "godot_cpp/classes/packed_scene.hpp"
#include "godot_cpp/classes/physics_server3d.hpp"
#include "godot_cpp/classes/property_tweener.hpp"
#include "godot_cpp/classes/sphere_mesh.hpp"
#include "godot_cpp/classes/standard_material3d.hpp"
#include "godot_cpp/classes/tween.hpp"
#include "godot_cpp/classes/world3d.hpp"
#include "godot_cpp/classes/resource_loader.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/core/math_defs.hpp"
#include "godot_cpp/classes/texture2d.hpp"
#include "game/game_rules_laniakea.h"
#include "godot_cpp/variant/transform3d.hpp"
#include "physics_layers.h"

CVar WeaponCounterShield::weapon_counter_shield_offset_cvar = CVar::create_variable("weapon_counter_shield_offset", GDEXTENSION_VARIANT_TYPE_VECTOR3, Vector3(), "Offset for the counter shield");
CVar WeaponCounterShield::weapon_counter_shield_radius_cvar = CVar::create_variable("weapon_counter_shield_radius", GDEXTENSION_VARIANT_TYPE_FLOAT, 1.0f, "Radius for the counter shield");

Vector3 WeaponCounterShield::_get_shield_center(BaseCharacter *p_character) const {
    return p_character->get_model()->get_global_transform().xform(weapon_counter_shield_offset_cvar.get_vector3());
}

void WeaponCounterShield::_update_shield_visuals(float p_animation_progress, BaseCharacter *p_character) {
    shield_visuals.node->set_global_position(_get_shield_center(p_character));
    const Basis character_basis = p_character->get_model()->get_global_basis();
    const Vector3 character_forward_axis = character_basis.xform(Vector3(0.0f, 0.0f, -1.0f));
    shield_visuals.node->set_basis(Basis::looking_at(character_forward_axis));
    const float shield_diameter = weapon_counter_shield_radius_cvar.get_float() * 2.0f * p_animation_progress;
    shield_visuals.node->scale_object_local(Vector3(shield_diameter, shield_diameter, shield_diameter));
}

void WeaponCounterShield::primary_attack(int p_weapon_slot, const WeaponButtonState &p_button_state, BaseCharacter *p_character)
{
    
}

void WeaponCounterShield::post_update(int p_weapon_slot, BaseCharacter *p_character, const WeaponButtonState &p_button_state) {
    const Basis character_basis = p_character->get_model()->get_global_basis();
    const Vector3 character_forward_axis = character_basis.xform(Vector3(0.0f, 0.0f, -1.0f));

    if (shield_visuals.node != nullptr) {
        PhysicsServer3D *ps = PhysicsServer3D::get_singleton();
        if (p_button_state.fire) {


            ps->body_set_state(shield_physics_body, PhysicsServer3D::BODY_STATE_TRANSFORM, shield_visuals.node->get_global_transform());
            shield_shape->get_debug_mesh()->surface_set_material(0, memnew(StandardMaterial3D));
            DebugOverlay::mesh_with_trf(shield_visuals.node->get_global_transform(), shield_shape->get_debug_mesh(), true, 0.0f);

            const bool shield_saturated = collected_bullets.size() >= MAX_BULLET_COUNT;
            ps->body_set_shape_disabled(shield_physics_body, 0, shield_saturated);

            if (!shield_visuals.node->is_visible()) {
                if (shield_visuals.tween.is_valid()) {
                    shield_visuals.tween->kill();
                    shield_visuals.tween.unref();
                }
                shield_visuals.tween = p_character->create_tween();
                shield_visuals.tween->tween_method(callable_mp(this, &WeaponCounterShield::_update_shield_visuals).bind(p_character), 0.0, 1.0f, 0.25f) \
                    ->set_ease(Tween::EASE_OUT) \
                    ->set_trans(godot::Tween::TRANS_BOUNCE);

                shield_visuals.node->show();
            }

            if (shield_visuals.tween.is_null() || !shield_visuals.tween->is_running()) {
                _update_shield_visuals(1.0f, p_character);
            }
        } else {
            shield_visuals.node->hide();
            ps->body_set_shape_disabled(shield_physics_body, 0, true);
        }


    }

    if (!p_button_state.fire && !collected_bullets.is_empty()) {

        const float shield_radius = weapon_counter_shield_radius_cvar.get_float();
        const Vector3 shield_center = _get_shield_center(p_character);

        Vector3 occlusion_target_pos = shield_center + character_forward_axis * get_max_distance();
        if (PlayerCharacter *player = Object::cast_to<PlayerCharacter>(p_character)) {
            player->get_occlusion_target_position(BaseCharacter::WEAPON_SLOT_SECONDARY, occlusion_target_pos);
        }

        // fire collected bullets from random positions in the shield
        for (int i = 0; i < collected_bullets.size(); i++) {
            const Vector3 bullet_pos = collected_bullets[i].mesh->get_global_position();
            p_character->fire_bullet(bullet_pos, bullet_pos.direction_to(occlusion_target_pos), get_max_distance(), LaniakeaGameRules::RIFLE_AMMO_TYPE, 10.0f, true);
            collected_bullets[i].mesh->queue_free();
        }

        collected_bullets.clear();
    }
}

float WeaponCounterShield::get_max_distance() const {
    return 1000.0f;
}

StringName WeaponCounterShield::get_item_name() const {
    static StringName s = "weapon_counter_shield_item";
    return s;
}
StringName WeaponCounterShield::_get_weapon_name() {
    return "weapon_counter_shield";
};

bool WeaponCounterShield::uses_occluded_crosshair() const {
    return collected_bullets.size()  > 0;
}

void WeaponCounterShield::equipped(int p_weapon_slot, BaseCharacter *p_character) {
    /*MeshInstance3D *mi = memnew(MeshInstance3D);
    Ref<SphereMesh> sphere_mesh;
    sphere_mesh.instantiate();
    sphere_mesh->set_radius(weapon_counter_shield_radius_cvar.get_float());
    sphere_mesh->set_height(weapon_counter_shield_radius_cvar.get_float() * 2.0f);

    Ref<StandardMaterial3D> mat;
    mat.instantiate();
    mat->set_albedo(Color(0.0, 0.0, 1.0, 0.1));
    mat->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);

    sphere_mesh->set_material(mat);

    mi->set_mesh(sphere_mesh);
    p_character->add_child(mi);*/

    Ref<PackedScene> bubble_scene = ResourceLoader::get_singleton()->load("res://scenes/bubble_shield.tscn");

    DEV_ASSERT(bubble_scene.is_valid());
    shield_visuals.node = Object::cast_to<Node3D>(bubble_scene->instantiate());
    p_character->add_child(shield_visuals.node);

    PhysicsServer3D *ps = PhysicsServer3D::get_singleton();

    shield_physics_body = ps->body_create();

    if (shield_shape.is_null()) {
        shield_shape.instantiate();
        const float shield_radius = weapon_counter_shield_radius_cvar.get_float();
        shield_shape->set_size(Vector3(shield_radius, shield_radius, 0.1f));
    }
    
    ps->body_add_shape(shield_physics_body, shield_shape->get_rid());
    ps->body_attach_object_instance_id(shield_physics_body, get_instance_id());
    ps->body_set_collision_mask(shield_physics_body, 0);
    ps->body_set_collision_layer(shield_physics_body, PhysicsLayers::LAYER_ENTITY_HITBOXES);
    ps->body_set_space(shield_physics_body, p_character->get_world_3d()->get_space());

    bullet_shape.instantiate();
    bullet_shape->set_height(0.05f);
    bullet_shape->set_radius(0.01f);

    Ref<StandardMaterial3D> bullet_mat;
    bullet_mat.instantiate();
    bullet_mat->set_albedo(Color(1.0, 1.0, 0.0));

    bullet_shape->set_material(bullet_mat);

    if (PlayerCharacter *character = Object::cast_to<PlayerCharacter>(p_character)) {
        character->add_aim_occlusion_exception(shield_physics_body);
    }
}

void WeaponCounterShield::unequipped(int p_weapon_slot, BaseCharacter *p_character) {
    if (!collected_bullets.is_empty()) {
        shield_visuals.node->queue_free();
        shield_visuals.node = nullptr;
    }

    if (shield_physics_body.is_valid()) {
        if (PlayerCharacter *character = Object::cast_to<PlayerCharacter>(p_character)) {
            character->remove_occlusion_exception(shield_physics_body);
        }
        PhysicsServer3D::get_singleton()->free_rid(shield_physics_body);
        shield_physics_body = RID();
    }
}

void WeaponCounterShield::on_bullet_damage_received(int p_ammo_type, float p_damage, const Vector3 &p_position, const Vector3 &p_normal, int p_shape_idx) {
    MeshInstance3D *bullet_mi = memnew(MeshInstance3D);
    bullet_mi->set_mesh(bullet_shape);
    shield_visuals.node->add_child(bullet_mi);

    const float shield_radius = weapon_counter_shield_radius_cvar.get_float();
    
    const float theta = UtilityFunctions::randf_range(-Math_PI, Math_PI);
    Vector3 pos_base = shield_visuals.node->to_local(p_position);
    const float r = UtilityFunctions::randf_range(-0.1f, 0.1f);
    const Vector3 pos = pos_base + Vector3(0.0, r, 0.0).rotated(Vector3(0.0, 0.0, -1.0f), theta);
    
    bullet_mi->set_position(pos);

    collected_bullets.push_back({
        .mesh = bullet_mi
    });
}
