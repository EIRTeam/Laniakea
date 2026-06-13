#include "player_ui.h"

#include "game/base_character.h"
#include "game/game_rules.h"
#include "game/main_loop.h"
#include "game/ui/item_selector_ui.h"
#include "game/weapon_instance.h"
#include "game_rules_laniakea.h"
#include "godot_cpp/classes/camera3d.hpp"
#include "godot_cpp/classes/canvas_item.hpp"
#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/classes/input.hpp"
#include "godot_cpp/classes/node2d.hpp"
#include "player_character.h"
#include "ui/magic_get_node.h"

#include <optional>

void PlayerUI::notify_equip_item_requested(StringName p_item_name, int p_slot) {
	print_line("EQUIP ME DADDY");
	emit_signal("equip_item_requested", p_slot, LaniakeaMainLoop::get_singleton()->get_game_rules()->weapon_from_item_name(p_item_name));
	if (current_selector_ui != nullptr) {
		current_selector_ui->queue_free();
		current_selector_ui = nullptr;
	}
}

void PlayerUI::notify_unequip_item_requested(int p_slot) {
	emit_signal("unequip_item_requested", p_slot);
	if (current_selector_ui != nullptr) {
		current_selector_ui->queue_free();
		current_selector_ui = nullptr;
	}
}

void PlayerUI::notify_firearm_equipped(const Ref<WeaponFirearmInstance> &p_weapon, int p_slot, int p_chambered_ammo, int p_ammo_pool_total) {
	if (p_slot != BaseCharacter::WEAPON_SLOT_PRIMARY) {
		return;
	}
	ammo_label->set_visible(p_weapon.is_valid());

	if (p_weapon.is_valid()) {
		ammo_label->set_text(vformat("%d/%d", p_chambered_ammo, p_ammo_pool_total));
	}
}

void PlayerUI::notify_firearm_ammo_spent(int p_slot, int p_chambered_ammo, int p_ammo_pool_total) {
	if (p_slot != BaseCharacter::WEAPON_SLOT_PRIMARY) {
		return;
	}

	ammo_label->set_text(vformat("%d/%d", p_chambered_ammo, p_ammo_pool_total));
}

void PlayerUI::notify_firearm_ammo_acquired(int p_slot, int p_chambered_ammo, int p_ammo_pool_total) {
	if (p_slot != BaseCharacter::WEAPON_SLOT_PRIMARY) {
		return;
	}

	ammo_label->set_text(vformat("%d/%d", p_chambered_ammo, p_ammo_pool_total));
}

void PlayerUI::notify_firearm_reloaded(int p_slot, int p_chambered_ammo, int p_ammo_pool_total) {
	if (p_slot != BaseCharacter::WEAPON_SLOT_PRIMARY) {
		return;
	}

	ammo_label->set_text(vformat("%d/%d", p_chambered_ammo, p_ammo_pool_total));
}

void PlayerUI::_bind_methods() {
	ADD_SIGNAL(MethodInfo("equip_item_requested", PropertyInfo(Variant::INT, "slot"), PropertyInfo(Variant::OBJECT, "item", PROPERTY_HINT_RESOURCE_TYPE, "WeaponInstanceBase")));
	ADD_SIGNAL(MethodInfo("unequip_item_requested", PropertyInfo(Variant::INT, "slot")));
}

void PlayerUI::_ready() {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	crosshairs_container = get_node<Control>("%Crosshairs");
	occlusion_crosshairs_container = get_node<Control>("%OcclusionCrosshairs");
	ammo_label = get_node<Label>("%AmmoLabel");
}

void PlayerUI::update(PlayerCharacter *p_player, float p_delta) {
	static StringName primary_item_action = "select_primary_item";
	static StringName secondary_item_action = "select_secondary_item";
	Input *input = Input::get_singleton();
	const bool is_primary_item_action_just_pressed = input->is_action_just_pressed(primary_item_action);
	const bool is_primary_item_action_pressed = input->is_action_pressed(primary_item_action);
	const bool is_secondary_item_action_pressed = input->is_action_pressed(secondary_item_action);
	const bool is_secondary_item_action_just_pressed = input->is_action_just_pressed(secondary_item_action);

	if (current_selector_ui == nullptr && (is_primary_item_action_just_pressed || is_secondary_item_action_just_pressed)) {
		current_selector_ui = instantiate_scene_type_checked<ItemSelectorUI>("res://scenes/ui/item_selector/item_selector_ui.tscn");
		const BaseCharacter::WeaponSlot slot = is_primary_item_action_pressed ? BaseCharacter::WEAPON_SLOT_PRIMARY : BaseCharacter::WEAPON_SLOT_SECONDARY;

		std::optional<StringName> current_weapon;

		if (Ref<WeaponInstanceBase> weapon = p_player->get_equipped_weapon(slot); weapon.is_valid()) {
			current_weapon = weapon->get_item_name();
		}

		add_child(current_selector_ui);

		Input::get_singleton()->set_mouse_mode(Input::MOUSE_MODE_VISIBLE);
		current_selector_ui->show_items(p_player->get_available_weapon_items(slot), current_weapon);
		current_selector_ui->connect("equip_requested", callable_mp(this, &PlayerUI::notify_equip_item_requested).bind(slot));
		current_selector_ui->connect("unequip_requested", callable_mp(this, &PlayerUI::notify_unequip_item_requested).bind(slot));

	} else if (current_selector_ui != nullptr && !(is_primary_item_action_pressed || is_secondary_item_action_pressed)) {
		current_selector_ui->queue_free();
		current_selector_ui = nullptr;
	}
}

void PlayerUI::update_crosshair(int p_slot, Camera3D *p_camera, bool p_show_crosshair, bool p_occluded, Vector3 p_occlusion_position) {
	ERR_FAIL_INDEX(p_slot, crosshairs_container->get_child_count());
	ERR_FAIL_INDEX(p_slot, occlusion_crosshairs_container->get_child_count());

	Node2D *crosshair = Object::cast_to<Node2D>(crosshairs_container->get_child(p_slot));
	Node2D *occlusion_crosshair = Object::cast_to<Node2D>(occlusion_crosshairs_container->get_child(p_slot));

	ERR_FAIL_COND(crosshair == nullptr);
	ERR_FAIL_COND(occlusion_crosshair == nullptr);

	crosshair->set_visible(p_show_crosshair);
	occlusion_crosshair->set_visible(p_show_crosshair && p_occluded && !p_camera->is_position_behind(p_occlusion_position));
	if (occlusion_crosshair->is_visible()) {
		occlusion_crosshair->set_global_position(p_camera->unproject_position(p_occlusion_position));
	}
}
