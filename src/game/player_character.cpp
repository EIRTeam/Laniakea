#include "player_character.h"

#include "bind_macros.h"
#include "debug/debug_constexpr.h"
#include "debug/debug_overlay.h"
#include "game/base_character.h"
#include "game/biped_animation_base.h"
#include "game/main_loop.h"
#include "game/movement_settings.h"
#include "game/movement_shared.h"
#include "game/weapon_counter_shield.h"
#include "game/weapon_firearm.h"
#include "game/weapon_gravitygun.h"
#include "game/weapon_instance.h"
#include "game/weapon_model.h"
#include "game/weapon_rifle_test.h"
#include "gdextension_interface.h"
#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/classes/input.hpp"
#include "godot_cpp/classes/packed_scene.hpp"
#include "godot_cpp/classes/physics_direct_space_state3d.hpp"
#include "godot_cpp/classes/physics_ray_query_parameters3d.hpp"
#include "godot_cpp/classes/ref.hpp"
#include "godot_cpp/classes/resource_loader.hpp"
#include "godot_cpp/classes/window.hpp"
#include "godot_cpp/classes/world3d.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/variant/callable_method_pointer.hpp"
#include "godot_cpp/variant/packed_vector3_array.hpp"
#include "godot_cpp/variant/transform3d.hpp"
#include "physics_layers.h"

#include <algorithm>

CVar PlayerCharacter::player_camera_horizontal_deadzone_radius = CVar::create_variable("player.camera_horizontal_deadzone_radius", GDEXTENSION_VARIANT_TYPE_FLOAT, 0.1f, "Camera horizontal deadzone when not aiming.", PROPERTY_HINT_NONE, "");
CVar PlayerCharacter::player_camera_distance_aim = CVar::create_variable("player.camera_distance_aim", GDEXTENSION_VARIANT_TYPE_FLOAT, 1.5f, "Camera distance when aiming.", PROPERTY_HINT_NONE, "");
CVar PlayerCharacter::player_camera_distance = CVar::create_variable("player.camera_distance", GDEXTENSION_VARIANT_TYPE_FLOAT, 2.5f, "Camera distance when not aiming.", PROPERTY_HINT_NONE, "");
CVar PlayerCharacter::player_get_primary_ammo_command = CVar::create_command("player.get_primary_ammo", "Get ammo for the primary weapon", { CVar::DelayedPropertyInfo(Variant::INT, "amount") });

void PlayerCharacter::_on_weapon_equipped(int p_slot, Ref<WeaponInstanceBase> p_weapon) {
	if (Ref<WeaponFirearmInstance> firearm = p_weapon; firearm.is_valid()) {
		player_ui->notify_firearm_equipped(p_weapon, p_slot, get_ammo_in_weapon_clip(p_weapon->get_weapon_name()), get_remaining_ammo_in_pool(firearm->get_ammo_type()));
	} else {
		player_ui->notify_firearm_equipped(p_weapon, p_slot, -1, -1);
	}
}

void PlayerCharacter::_on_weapon_reloaded(int p_slot, Ref<WeaponInstanceBase> p_weapon) {
	if (Ref<WeaponFirearmInstance> firearm = p_weapon; firearm.is_valid()) {
		player_ui->notify_firearm_reloaded(p_slot, get_ammo_in_weapon_clip(p_weapon->get_weapon_name()), get_remaining_ammo_in_pool(firearm->get_ammo_type()));
	}
}

void PlayerCharacter::_on_weapon_ammo_used(int p_slot, Ref<WeaponInstanceBase> p_weapon) {
	if (Ref<WeaponFirearmInstance> firearm = p_weapon; firearm.is_valid()) {
		player_ui->notify_firearm_ammo_spent(p_slot, get_ammo_in_weapon_clip(p_weapon->get_weapon_name()), get_remaining_ammo_in_pool(firearm->get_ammo_type()));
	}
}

void PlayerCharacter::_notify_ammo_acquired(int p_ammo_type) {
	if (!equipped_weapons[WEAPON_SLOT_PRIMARY].is_valid()) {
		return;
	}
	if (Ref<WeaponFirearmInstance> firearm = equipped_weapons[WEAPON_SLOT_PRIMARY]; firearm.is_valid()) {
		if (firearm->get_ammo_type() == p_ammo_type) {
			player_ui->notify_firearm_ammo_spent(WEAPON_SLOT_PRIMARY, get_ammo_in_weapon_clip(firearm->get_weapon_name()), get_remaining_ammo_in_pool(firearm->get_ammo_type()));
		}
	}
}

void PlayerCharacter::get_primary_ammo_command(int p_amount) {
	Ref<WeaponInstanceBase> weapon = get_equipped_weapon(WEAPON_SLOT_PRIMARY);
	Ref<WeaponFirearmInstance> firearm = weapon;

	if (firearm.is_valid()) {
		const int ammo_type = firearm->get_ammo_type();
		ERR_FAIL_INDEX(ammo_type, character_state.ammo_pools.size());
		character_state.ammo_pools[ammo_type] += p_amount;
		_notify_ammo_acquired(ammo_type);
	}
}

void PlayerCharacter::_bind_methods() {
	MAKE_BIND_NODE(PlayerCharacter, camera_offset_target, Node3D);
	MAKE_BIND_NODE(PlayerCharacter, player_ui, PlayerUI);
}

Vector2 PlayerCharacter::get_movement_vector_transformed() const {
	Vector2 input_vector = get_movement_vector();
	return camera->transform_input(input_vector);
}

BitField<BaseCharacter::InputActionState> PlayerCharacter::get_action_state(InputCommand p_command) const {
	ERR_FAIL_INDEX_V(p_command, input_state.button_states.size(), 0);
	return input_state.button_states[p_command];
}

PlayerCharacter::PlayerCharacter() {
}

void PlayerCharacter::_ready() {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}

	player_get_primary_ammo_command.connect_command_callback(callable_mp(this, &PlayerCharacter::get_primary_ammo_command));

	Ref<WeaponRifleTest> rifle_test;
	Ref<WeaponGravityGun> gravity_gun_milk;
	rifle_test.instantiate();
	gravity_gun_milk.instantiate();
	equip_weapon(WEAPON_SLOT_PRIMARY, rifle_test);
	equip_weapon(WEAPON_SLOT_SECONDARY, gravity_gun_milk);

	movement_settings = get_movement_settings();

	camera = memnew(PlayerCamera);
	add_child(camera);
	camera->set_as_top_level(true);

	for (int i = 0; i < WEAPON_SLOT_MAX; i++) {
		SlotAimOcclusionInformation &occlusion_info = per_slot_aim_occlusion_info[i];
		occlusion_info.target_position_interp_node = memnew(Node3D);
		add_child(occlusion_info.target_position_interp_node);
		occlusion_info.target_position_interp_node->set_as_top_level(true);
		occlusion_info.target_position_interp_node->set_physics_interpolation_mode(Node::PHYSICS_INTERPOLATION_MODE_ON);
	}

	if (player_ui) {
		player_ui->connect("unequip_item_requested", callable_mp(static_cast<BaseCharacter *>(this), &BaseCharacter::equip_weapon).bind(Ref<WeaponInstanceBase>()));
		player_ui->connect("equip_item_requested", callable_mp(static_cast<BaseCharacter *>(this), &BaseCharacter::equip_weapon));
	}

	BaseCharacter::_ready();

	player_animation = Object::cast_to<BipedAnimationBase>(animation);

	player_animation->set_weapon_animation_set(BipedAnimationBase::WEAPON_ANIMATION_TYPE_RIFLE);

	add_aim_occlusion_exception(get_hitbox_detector_body_rid());
}

void PlayerCharacter::_process(double p_delta) {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}

	const bool is_aiming = is_action_pressed(InputCommand::AIM);

	camera->set_enable_leash(!is_aiming);
	camera->set_horizontal_deadzone_radius(is_aiming ? 0.0f : player_camera_horizontal_deadzone_radius.get_float());

	if (camera_offset_target != nullptr) {
		camera->update(movement.get_effective_velocity(), camera_offset_target->get_global_transform_interpolated().origin);
	} else {
		camera->update(movement.get_effective_velocity(), get_global_transform_interpolated().origin);
	}

	_ui_process(p_delta);

	Vector3 camera_aim_origin;
	Vector3 camera_aim_normal;
	get_camera_aim_trajectory(camera_aim_origin, camera_aim_normal);

	player_animation->set_aim_x_angle(-(camera_aim_normal.angle_to(Vector3(0.0f, 1.0f, 0.0f)) - Math::deg_to_rad(90.0f)));

	player_animation->set_is_aiming(is_aiming && equipped_weapons[WEAPON_SLOT_PRIMARY].is_valid());

	BaseCharacter::_process(p_delta);
}

void PlayerCharacter::_physics_process(double p_delta) {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	static StringName move_left = "move_left";
	static StringName move_right = "move_right";
	static StringName move_forward = "move_forward";
	static StringName move_back = "move_back";
	static StringName primary_fire = "primary_fire";
	static StringName secondary_fire = "secondary_fire";
	static StringName sprint = "sprint";
	static StringName aim = "aim";

	const bool has_any_weapon = std::any_of(equipped_weapons.begin(), equipped_weapons.end(), [](const Ref<WeaponInstanceBase> &inst) {
		return inst.is_valid();
	});

	input_state.movement_input = Input::get_singleton()->get_vector(move_left, move_right, move_forward, move_back);
	input_state.button_states[InputCommand::AIM] = has_any_weapon ? _get_action_state(aim) : BitField<InputActionState>(0); // Bit of a HACK but ehhh
	input_state.button_states[InputCommand::PRIMARY_FIRE] = _get_action_state(primary_fire);
	input_state.button_states[InputCommand::SECONDARY_FIRE] = _get_action_state(secondary_fire);
	input_state.button_states[InputCommand::SPRINT] = _get_action_state(sprint);

	_movement_physics_process(p_delta);
	BaseCharacter::_physics_process(p_delta);
}

void PlayerCharacter::_movement_physics_process(float p_delta) {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}

	if (Input::get_singleton()->is_mouse_button_pressed(MOUSE_BUTTON_LEFT)) {
		Input::get_singleton()->set_mouse_mode(Input::MOUSE_MODE_CAPTURED);
	}

	const bool is_sprinting = is_action_pressed(InputCommand::SPRINT);

	const bool is_aiming = is_action_pressed(InputCommand::AIM);

	for (int slot_i = 0; slot_i < WeaponSlot::WEAPON_SLOT_MAX; slot_i++) {
		Ref<WeaponInstanceBase> equipped_weapon = equipped_weapons[slot_i];
		if (!equipped_weapon.is_valid() || !is_aiming) {
			continue;
		}
		// Time to do something slightly funny
		// since we are in third person, we want to cast a ray from our firing position to wherever we are looking at
		// this will help us set up our separate occlusion crosshair

		Vector3 camera_aim_origin;
		Vector3 camera_aim_normal;
		get_camera_aim_trajectory(camera_aim_origin, camera_aim_normal);

		Ref<PhysicsRayQueryParameters3D> ray_query;
		ray_query.instantiate();
		ray_query->set_from(camera_aim_origin);
		ray_query->set_to(camera_aim_origin + camera_aim_normal * 1000.0f);
		ray_query->set_collision_mask(PhysicsLayers::LAYER_WORLDSPAWN | PhysicsLayers::LAYER_PROPS | PhysicsLayers::LAYER_ENTITY_HITBOXES);
		ray_query->set_exclude(occlusion_exceptions);

		PhysicsDirectSpaceState3D *dss = get_world_3d()->get_direct_space_state();
		const Vector3 firing_position = get_firing_position(slot_i);

		Vector3 occlusion_check_target;

		if (Dictionary camera_ray_out = dss->intersect_ray(ray_query); !camera_ray_out.is_empty()) {
			occlusion_check_target = camera_ray_out["position"];
			DebugOverlay::sphere(camera_ray_out["position"], 0.1f, Color(0.0, 1.0, 0.0f));
			DebugOverlay::line(firing_position, camera_ray_out["position"], Color(0.0, 1.0, 0.0));
		} else {
			occlusion_check_target = ray_query->get_to();
		}

		ray_query->set_from(firing_position);
		const Vector3 diff = occlusion_check_target - firing_position;
		ray_query->set_to(firing_position + diff.limit_length(MAX(diff.length() - 0.25f, 0.15f)));

		SlotAimOcclusionInformation &occlusion_info = per_slot_aim_occlusion_info[slot_i];

		Dictionary occlusion_check_out = dss->intersect_ray(ray_query);

		if (!occlusion_check_out.is_empty()) {
			occlusion_info.aim_target_direction = ray_query->get_from().direction_to(ray_query->get_to());

			occlusion_info.target_position_interp_node->set_global_position(occlusion_check_out["position"]);
			if (slot_i == WEAPON_SLOT_SECONDARY) {
				DebugOverlay::sphere(occlusion_check_out["position"], 0.1f, Color(1.0, 0.0, 0.0f));
				DebugOverlay::line(firing_position, occlusion_check_out["position"], Color(0.0, 1.0, 0.0));
			}
			// Reset the physics interpolation if needed
			if (!occlusion_info.is_target_position_occluded) {
				occlusion_info.target_position_interp_node->reset_physics_interpolation();
			}
			occlusion_info.is_target_position_occluded = true;
		} else {
			occlusion_info.aim_target_direction = firing_position.direction_to(ray_query->get_to());
			occlusion_info.is_target_position_occluded = false;
			occlusion_info.target_position_interp_node->set_global_position(ray_query->get_to());

			DebugOverlay::sphere(ray_query->get_to(), 0.1f, Color("PURPLE"));
		}
	}

	set_facing_direction_mode(is_aiming ? FacingDirectionMode::CUSTOM : FacingDirectionMode::TO_MOVEMENT);

	if (model) {
		if (is_aiming) {
			// strafe
			Vector3 target_facing_dir = camera->get_camera()->get_global_basis().xform(Vector3(0.0, 0.0, -1.0f));
			target_facing_dir.y = 0.0f;
			target_facing_dir.normalize();

			if (target_facing_dir.is_normalized()) {
				model->set_target_facing_direction(target_facing_dir);
			}
		} else {
			Vector3 normalized_vel = movement.get_effective_velocity();
			normalized_vel.y = 0.0f;
			normalized_vel.normalize();

			if (normalized_vel.is_normalized()) {
				model->set_target_facing_direction(normalized_vel);
			}
		}
	}

	DebugOverlay::horz_arrow(get_global_position(), model->get_target_facing_direction() * 1.0f + get_global_position(), 0.25f, Color::named("Green"));

	const Vector2 target_camera_framing = is_aiming ? Vector2(0.35, 0.0) : Vector2();
	const float target_camera_distance = is_aiming ? player_camera_distance_aim.get_float() : player_camera_distance.get_float();

	camera->set_framing(target_camera_framing, false);
	camera->set_distance(target_camera_distance, false);
}

void PlayerCharacter::_ui_process(float p_delta) {
	// Update UI
	const bool is_aiming = is_action_pressed(InputCommand::AIM);

	if (player_ui != nullptr) {
		player_ui->update(this, p_delta);
		for (int i = 0; i < WEAPON_SLOT_MAX; i++) {
			const SlotAimOcclusionInformation &occlusion_info = per_slot_aim_occlusion_info[i];
			player_ui->update_crosshair(i, camera->get_camera(), is_aiming, occlusion_info.is_target_position_occluded, occlusion_info.target_position_interp_node->get_global_transform_interpolated().origin);
		}
	}
}

BitField<BaseCharacter::InputActionState> PlayerCharacter::_get_action_state(const StringName p_state) const {
	BitField<BaseCharacter::InputActionState> state = 0;
	Input *input = Input::get_singleton();
	if (input->is_action_just_released(p_state)) {
		state.set_flag(InputActionState::JUST_RELEASED);
	} else if (input->is_action_just_pressed(p_state)) {
		state.set_flag(InputActionState::JUST_PRESSED);
		state.set_flag(InputActionState::PRESSED);
	} else if (input->is_action_pressed(p_state)) {
		state.set_flag(InputActionState::PRESSED);
	}

	return state;
}

void PlayerCharacter::_camera_process(float p_delta) {
}

void PlayerCharacter::get_camera_aim_trajectory(Vector3 &r_origin, Vector3 &r_direction) const {
	const Vector2 screen_center = Vector2(get_window()->get_size()) * 0.5f;
	r_origin = camera->get_camera()->project_ray_origin(screen_center);
	r_direction = camera->get_camera()->project_ray_normal(screen_center);
}

void PlayerCharacter::get_aim_trajectory(int p_weapon_slot, Vector3 &r_origin, Vector3 &r_direction) {
	ERR_FAIL_INDEX(p_weapon_slot, WEAPON_SLOT_MAX);
	r_origin = get_firing_position(p_weapon_slot);
	r_direction = per_slot_aim_occlusion_info[p_weapon_slot].aim_target_direction;
}

void PlayerCharacter::add_camera_kick(float p_max_vertical_kick_angle, float p_fire_duration_time, float p_slide_limit_time) {
	camera->do_machine_gun_kick(p_max_vertical_kick_angle, p_fire_duration_time, p_slide_limit_time);
}

Ref<MovementSettings> PlayerCharacter::get_movement_settings() const {
	Ref<MovementSettings> settings = ResourceLoader::get_singleton()->load("res://data/player_movement.tres");
	ERR_FAIL_COND_V(!settings.is_valid(), BaseCharacter::get_movement_settings());
	return settings;
}

Vector3 PlayerCharacter::get_slot_target_position(int p_slot) const {
	ERR_FAIL_INDEX_V(p_slot, WEAPON_SLOT_MAX, Vector3());
	return per_slot_aim_occlusion_info[p_slot].target_position_interp_node->get_global_position();
}

Transform3D PlayerCharacter::get_milk_attachment_transform() const {
	if (model->get_milk_attachment_point()) {
		return model->get_milk_attachment_point()->get_global_transform();
	}

	return Transform3D();
}

void PlayerCharacter::add_aim_occlusion_exception(RID p_exception) {
	occlusion_exceptions.push_back(p_exception);
}

void PlayerCharacter::remove_occlusion_exception(RID p_exception) {
	occlusion_exceptions.erase(p_exception);
}

bool PlayerCharacter::get_occlusion_target_position(WeaponSlot p_slot, Vector3 &r_target_pos) const {
	r_target_pos = per_slot_aim_occlusion_info[p_slot].target_position_interp_node->get_global_position();
	return per_slot_aim_occlusion_info[p_slot].is_target_position_occluded;
}

Vector3 PlayerCharacter::get_look_direction() const {
	return get_model()->get_eye_position().direction_to(per_slot_aim_occlusion_info[WEAPON_SLOT_PRIMARY].target_position_interp_node->get_global_position());
}

Vector<StringName> PlayerCharacter::get_available_weapon_items(WeaponSlot p_slot) const {
	Vector<StringName> weapon_items;
	// TODO: Hardcode this... for now
	if (p_slot == WEAPON_SLOT_PRIMARY) {
		weapon_items.push_back("weapon_rifle_test_item");
	} else if (p_slot == WEAPON_SLOT_SECONDARY) {
		weapon_items.push_back("weapon_gravitygun_item");
		weapon_items.push_back("weapon_counter_shield_item");
	}

	return weapon_items;
}

Vector2 PlayerCharacter::get_movement_vector() const {
	return input_state.movement_input;
}

BipedAnimationBase *PlayerCharacter::create_animation() const {
	return memnew(BipedAnimationBase);
}
