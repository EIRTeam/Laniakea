#include "character_hitbox.h"

#include "bind_macros.h"
#include "game/character_hitbox_detector.h"
#include "godot_cpp/classes/array_mesh.hpp"
#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/classes/skeleton3d.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/core/property_info.hpp"
#include "godot_cpp/variant/transform3d.hpp"

void CharacterHitbox::_editor_visual_update(const Transform3D &p_bone_pose) {
	if (get_detector() != nullptr) {
		Skeleton3D *skel = get_detector()->get_skeleton();
		if (skel == nullptr) {
			return;
		}
		set_transform(p_bone_pose * offset);
	}
}

CharacterHitboxDetector *CharacterHitbox::get_detector() const {
	return Object::cast_to<CharacterHitboxDetector>(get_parent());
}

int CharacterHitbox::get_bone_idx() const {
	ERR_FAIL_COND_V(!get_detector(), -1);
	ERR_FAIL_COND_V(!get_detector()->get_skeleton(), -1);
	if (!bone_idx_cache.has_value() && !bone_name.is_empty()) {
		int bone_idx = get_detector()->get_skeleton()->find_bone(bone_name);
		const_cast<CharacterHitbox *>(this)->bone_idx_cache = bone_idx;
	}

	return bone_idx_cache.value_or(-1);
}

void CharacterHitbox::_update_debug_color() {
	Color c = Color(1.0f, 0.0f, 0.0f, 1.0f);

	switch (hitbox_group) {
		case HEAD: {
			c = Color(0.0f, 1.0f, 0.0f);
		} break;
		case ARMS: {
			c = Color(1.0f, 0.0f, 0.0f);
		} break;
		case LEGS: {
			c = Color(0.0f, 0.0f, 1.0f);
		} break;
		case TORSO: {
			c = Color(1.0f, 1.0f, 0.0f);
		}
	}

	set_debug_color(c);
}

void CharacterHitbox::set_bone_name(StringName p_bone_name) {
	bone_name = p_bone_name;
	bone_idx_cache.reset();
}

StringName CharacterHitbox::get_bone_name() const {
	return bone_name;
}

void CharacterHitbox::set_offset_rotation(const Vector3 &p_rotation) {
	offset.basis.set_euler_scale(p_rotation, offset.basis.get_scale());
}

Vector3 CharacterHitbox::get_offset_rotation() const {
	return offset.basis.get_euler_normalized();
}

void CharacterHitbox::set_offset_translation(const Vector3 &p_position) {
	offset.origin = p_position;
}

Vector3 CharacterHitbox::get_offset_translation() const {
	return offset.origin;
}

void CharacterHitbox::_bind_methods() {
	BIND_SETTER_GETTER(CharacterHitbox, offset);
	BIND_SETTER_GETTER(CharacterHitbox, hitbox_group);
	ADD_PROPERTY(PropertyInfo(Variant::TRANSFORM3D, "offset", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR), "set_offset", "get_offset");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "hitbox_group", PROPERTY_HINT_ENUM, "Head,Arms,Legs,Torso"), "set_hitbox_group", "get_hitbox_group");

	MAKE_BIND_T(CharacterHitbox, offset_rotation, Variant::VECTOR3, PROPERTY_HINT_RANGE, "-360,360,0.1,or_less,or_greater,radians_as_degrees");
	MAKE_BIND_T(CharacterHitbox, offset_translation, Variant::VECTOR3, PROPERTY_HINT_RANGE, "-99999,99999,or_greater,or_less,hide_control,suffix:m");

	BIND_ENUM_CONSTANT(HEAD);
	BIND_ENUM_CONSTANT(ARMS);
	BIND_ENUM_CONSTANT(LEGS);
	BIND_ENUM_CONSTANT(TORSO);
}

void CharacterHitbox::update(const Transform3D &p_bone_pose) {
	_editor_visual_update(p_bone_pose);
}

void CharacterHitbox::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
		} break;
		case NOTIFICATION_PARENTED: {
		} break;
		case NOTIFICATION_PROCESS: {
		} break;
	}
}

void CharacterHitbox::_get_property_list(List<PropertyInfo> *p_list) const {
	if (get_detector() && get_detector()->get_skeleton()) {
		Skeleton3D *skel = get_detector()->get_skeleton();
		p_list->push_back(PropertyInfo(Variant::STRING_NAME, "bone_name", PROPERTY_HINT_ENUM, skel->get_concatenated_bone_names()));
	} else {
		p_list->push_back(PropertyInfo(Variant::STRING_NAME, "bone_name"));
	}
}

bool CharacterHitbox::_set(const StringName &p_name, const Variant &p_value) {
	static StringName bone_name = "bone_name";
	if (p_name == bone_name) {
		set_bone_name(p_value);
		return true;
	}
	return false;
}

bool CharacterHitbox::_get(const StringName &p_name, Variant &r_ret) const {
	static StringName bone_name = "bone_name";
	if (p_name == bone_name) {
		r_ret = get_bone_name();
		return true;
	}

	return false;
}

void CharacterHitbox::set_hitbox_group(HitboxGroup p_hitbox_group) {
	hitbox_group = p_hitbox_group;
	_update_debug_color();
}

CharacterHitbox::HitboxGroup CharacterHitbox::get_hitbox_group() const {
	return hitbox_group;
}

CharacterHitbox::CharacterHitbox() {
	_update_debug_color();
}

void CharacterHitbox::set_offset(Transform3D p_offset) {
	offset = p_offset;
}

Transform3D CharacterHitbox::get_offset() const {
	return offset;
}
