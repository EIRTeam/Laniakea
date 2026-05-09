#include "character_hitbox_detector.h"
#include "character_hitbox.h"
#include "godot_cpp/classes/physics_server3d.hpp"
#include "godot_cpp/classes/skeleton_modifier3d.hpp"
#include "physics_layers.h"

Skeleton3D *CharacterHitboxDetector::get_skeleton() const {
    return skeleton;
}

void CharacterHitboxDetector::notify_hit(const CharacterHitbox::HitboxGroup p_hitbox_group, const Vector3 &p_position, const Vector3 &p_normal, int p_ammo_type, float p_damage) {
    static StringName hit_received = "hit_received";
    emit_signal(hit_received, p_hitbox_group, p_position, p_normal, p_ammo_type, p_damage);
}

void CharacterHitboxDetector::_ready() {
    set_collision_layer(PhysicsLayers::LAYER_ENTITY_HITBOXES);
    set_collision_mask(0);
}

void CharacterHitboxDetector::update_pose_cache() {
    if (!skeleton) {
        return;
    }
    for (int i = 0; i < get_child_count(); i++) {
        CharacterHitbox *hitbox = Object::cast_to<CharacterHitbox>(get_child(i));
        if (!hitbox) {
            continue;
        }

        int bone = skeleton->find_bone(hitbox->get_bone_name());

        if (bone == -1) {
            continue;
        }

        global_pose_cache[hitbox->get_bone_name()] = skeleton->get_bone_global_pose(bone);
    }
}

void CharacterHitboxDetector::_bind_methods() {
    ADD_SIGNAL(MethodInfo("hit_received", PropertyInfo(Variant::INT, "hitbox_group"), PropertyInfo(Variant::VECTOR3, "position"), PropertyInfo(Variant::VECTOR3, "normal"), PropertyInfo(Variant::INT, "ammo_type"), PropertyInfo(Variant::FLOAT, "damage")));
}

void CharacterHitboxDetector::on_bullet_damage_received(int p_ammo_type, float p_damage, const Vector3 &p_position, const Vector3 &p_normal, int p_shape_idx) {
    RID shape_rid = PhysicsServer3D::get_singleton()->body_get_shape(get_rid(), p_shape_idx);
    CharacterHitbox *hb = nullptr;
    for (int i = 0; i < get_child_count(); i++) {
        hb = Object::cast_to<CharacterHitbox>(get_child(i));
        if (hb != nullptr) {
            break;
        }
    }

    DEV_ASSERT(hb != nullptr);
    
    notify_hit(hb->get_hitbox_group(), p_position, p_normal, p_ammo_type, p_damage);
}

CharacterHitboxDetector::CharacterHitboxDetector() {
    set_as_top_level(true);
    set_physics_process(true);
}

void CharacterHitboxDetector::_notification(int p_what) {
    switch(p_what) {
        case NOTIFICATION_PARENTED: {
            skeleton = Object::cast_to<Skeleton3D>(get_parent());
            if (skeleton) {
                SkeletonModifier3D *last_modifier = nullptr;
                for (int i = 0; i < skeleton->get_child_count(); i++) {
                    if (SkeletonModifier3D *modifier = Object::cast_to<SkeletonModifier3D>(skeleton->get_child(i))) {
                        last_modifier = modifier;
                    }
                }
                if (last_modifier) {
                    last_modifier->connect("modification_processed", callable_mp(this, &CharacterHitboxDetector::update_pose_cache));
                }
            }
        } break;
        case NOTIFICATION_PHYSICS_PROCESS: {
            if (!skeleton) {
                return;
            }
            set_global_transform(skeleton->get_global_transform());
            for (int i = 0; i < get_child_count(); i++) {
                CharacterHitbox *hitbox = Object::cast_to<CharacterHitbox>(get_child(i));
                if (!hitbox) {
                    continue;
                }

                auto it = global_pose_cache.find(hitbox->get_bone_name());
                if (it == global_pose_cache.end()) {
                    continue;
                }

                hitbox->update(it->value);
            }
        } break;
        case NOTIFICATION_UNPARENTED: {
            skeleton = nullptr;
        } break;
    }
}
