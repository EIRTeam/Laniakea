#include "item_select_icon.h"

#include "game/game_rules_laniakea.h"
#include "game/main_loop.h"
#include "godot_cpp/classes/control.hpp"
#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/classes/label.hpp"
#include "godot_cpp/classes/style_box.hpp"
#include "godot_cpp/classes/texture_rect.hpp"
#include "godot_cpp/classes/v_box_container.hpp"
#include "ui/magic_get_node.h"

void ItemSelectIcon::_ready() {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	texture_rect = get_node_type_checked<TextureRect>(this, "%ItemIcon");
	name_label = get_node_type_checked<Label>(this, "%ItemNameLabel");
	set_theme_type_variation("ItemSelectIcon");
}

void ItemSelectIcon::initialize(StringName p_item_name) {
	DEV_ASSERT(is_node_ready());
	LaniakeaGameRules *game_rules = LaniakeaMainLoop::get_singleton()->get_game_rules();
	name_label->set_text(game_rules->item_get_localization_name(p_item_name));
	texture_rect->set_texture(game_rules->item_get_icon(p_item_name));
}

void ItemSelectIcon::set_hovered(bool p_hovered) {
	hovered = p_hovered;
	_update_stylebox();
}

void ItemSelectIcon::_update_stylebox() {
	static StringName panel_sname = StringName("panel");
	static StringName panel_selected_sname = StringName("panel_selected");
	static StringName hovered_sname = StringName("hovered");
	static StringName hovered_selected_sname = StringName("hovered_selected");
	remove_theme_stylebox_override(panel_sname);
	add_theme_stylebox_override(panel_sname, get_theme_stylebox(selected ? panel_selected_sname : panel_sname));
	if (hovered) {
		add_theme_stylebox_override(panel_sname, selected ? get_theme_stylebox(hovered_selected_sname) : get_theme_stylebox(hovered_sname));
	}
}

void ItemSelectIcon::_bind_methods() {
}

void ItemSelectIcon::set_selected(bool p_selected) {
	selected = p_selected;
	_update_stylebox();
}

bool ItemSelectIcon::get_selected() const {
	return selected;
}
