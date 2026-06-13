#pragma once

#include "bind_macros.h"
#include "game/damageable.h"
#include "godot_cpp/classes/collision_shape3d.hpp"
#include "godot_cpp/classes/node3d.hpp"
#include "godot_cpp/classes/shape3d.hpp"
#include "godot_cpp/core/binder_common.hpp"

#include <optional>

using namespace godot;

class CharacterHitboxDetector;

class CharacterHitbox : public CollisionShape3D {
	GDCLASS(CharacterHitbox, CollisionShape3D);

public:
	enum HitboxGroup {
		HEAD,
		ARMS,
		LEGS,
		TORSO
	};

private:
	StringName bone_name;
	HitboxGroup hitbox_group = HitboxGroup::ARMS;
	Transform3D offset;

	void _editor_visual_update(const Transform3D &p_bone_pose);
	CharacterHitboxDetector *get_detector() const;
	std::optional<int> bone_idx_cache;
	int get_bone_idx() const;
	void _update_debug_color();

public:
	void set_offset(Transform3D p_offset);
	Transform3D get_offset() const;

	void set_bone_name(StringName p_bone_name);
	StringName get_bone_name() const;

	void set_offset_rotation(const Vector3 &p_rotation);
	Vector3 get_offset_rotation() const;

	void set_offset_translation(const Vector3 &p_position);
	Vector3 get_offset_translation() const;

	static void _bind_methods();
	void update(const Transform3D &p_bone_pose);

	void _notification(int p_what);
	void _get_property_list(List<PropertyInfo> *p_list) const;
	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void set_hitbox_group(HitboxGroup p_category);
	HitboxGroup get_hitbox_group() const;
	CharacterHitbox();
};

VARIANT_ENUM_CAST(CharacterHitbox::HitboxGroup);
