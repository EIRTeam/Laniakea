#pragma once

#include "console/cvar.h"
#include "game/physics_prop.h"
#include "game/weapon_instance.h"
#include "godot_cpp/classes/generic6_dof_joint3d.hpp"
#include "godot_cpp/classes/static_body3d.hpp"

#include <optional>

class WeaponGravityGun : public WeaponInstanceBase {
	GDCLASS(WeaponGravityGun, WeaponInstanceBase);
	struct AttachedProp {
		float original_mass = 0.0f;
		bool could_sleep = false;
		ObjectID prop_object;
		StaticBody3D *attachment_body;
		Generic6DOFJoint3D *attachment_joint;
	};
	bool needs_attack_repress = false;
	std::optional<AttachedProp> attached_prop;

	static void _bind_methods() {}

	static CVar grabbed_offset_cvar;
	static CVar pull_force_cvar;
	static CVar pull_threshold_cvar;
	static CVar grab_threshold_cvar;
	static CVar throw_velocity_cvar;
	static CVar throw_angular_velocity_max_cvar;
	Vector3 _random_angular_velocity(float p_min_rads, float p_max_rads) const;
	Generic6DOFJoint3D *_create_joint() const;
	void grab_object(LaniakeaPhysicsProp *p_prop, BaseCharacter *p_character);
	void drop_current_object();
	void do_throw(BaseCharacter *p_character);

public:
	virtual void primary_attack(int p_weapon_slot, const WeaponButtonState &p_button_state, BaseCharacter *p_character) override;
	virtual void post_update(int p_weapon_slot, BaseCharacter *p_character, const WeaponButtonState &p_button_state) override;
	virtual float get_max_distance() const override;
	virtual StringName get_item_name() const override;
	static StringName _get_weapon_name() {
		return "weapon_gravitygun";
	}
};
