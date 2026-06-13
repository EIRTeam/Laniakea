#include "item_selector_ui.h"

#include "game/game_rules_laniakea.h"
#include "game/main_loop.h"
#include "game/ui/item_select_icon.h"
#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/classes/input_event_key.hpp"
#include "godot_cpp/classes/input_event_mouse_button.hpp"
#include "godot_cpp/classes/input_event_mouse_motion.hpp"
#include "godot_cpp/classes/label.hpp"
#include "godot_cpp/classes/mesh.hpp"
#include "godot_cpp/classes/mesh_instance2d.hpp"
#include "godot_cpp/classes/mesh_instance3d.hpp"
#include "godot_cpp/classes/method_tweener.hpp"
#include "godot_cpp/classes/quad_mesh.hpp"
#include "godot_cpp/classes/resource_saver.hpp"
#include "godot_cpp/classes/tween.hpp"
#include "godot_cpp/variant/packed_int32_array.hpp"
#include "radial_container.h"
#include "ui/magic_get_node.h"

void ItemSelectorUI::appear_animation_progress(float p_progress) {
	item_container->set_animation_progress(p_progress);
}

void ItemSelectorUI::_regenerate_fan_mesh() {
	PackedVector2Array vertices;
	PackedColorArray colors;
	PackedInt32Array indices;

	Ref<QuadMesh> quad_mesh;
	quad_mesh.instantiate();

	constexpr int RESOLUTION_WIDE = 5;
	constexpr int RESOLUTION_DEEP = 5;
	constexpr float MIN_DIST = 0.2f;

	quad_mesh->set_subdivide_width(RESOLUTION_WIDE);
	quad_mesh->set_subdivide_depth(RESOLUTION_DEEP);
	quad_mesh->set_center_offset(Vector3(0.5f, 0.0f, 0.0f));
	quad_mesh->request_update();

	Array mesh_arr = quad_mesh->get_mesh_arrays();

	PackedVector3Array vtx_array = mesh_arr[Mesh::ARRAY_VERTEX];
	Vector3 *vtx_array_ptrw = vtx_array.ptrw();

	const float rads_per_child = Math::deg_to_rad(25.0f);

	for (int i = 0; i < vtx_array.size(); i++) {
		Vector3 new_vertex = Vector3(
				UtilityFunctions::remap(vtx_array_ptrw[i].x, 0.0, 1.0, MIN_DIST, 1.0f),
				vtx_array_ptrw[i].y,
				vtx_array_ptrw[i].z);

		float vtx_angle_perc = Math::abs(new_vertex.y * 2.0f);
		vtx_angle_perc = Math::sqrt(vtx_angle_perc);
		vtx_angle_perc *= SIGN(new_vertex.y);

		const Vector2 vtx_2d = Vector2(new_vertex.x, 0.0f).rotated(vtx_angle_perc * rads_per_child);
		vtx_array_ptrw[i] = Vector3(vtx_2d.x, vtx_2d.y, 0.0f);
	}

	Ref<ArrayMesh> new_mesh;
	new_mesh.instantiate();
	mesh_arr[Mesh::ARRAY_VERTEX] = vtx_array;
	new_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, mesh_arr);

	fan_mesh.mesh = new_mesh;

	fan_mesh_instance->set_mesh(new_mesh);

	ResourceSaver::get_singleton()->save(new_mesh, "res://test_mesh2.tres");

	/*
		for i in range(arrs[Mesh.ARRAY_VERTEX].size()):
		var vertex = arrs[Mesh.ARRAY_VERTEX][i]
		vertex.x = remap(vertex.x, 0.0, 1.0, min_dist, 1.0)
		var vertex_2d = Vector2(vertex.x, 0.0)
		var vtx_angle := abs((vertex.y * 2.0)) as float
		vtx_angle = sqrt(vtx_angle)
		vtx_angle *= sign(vertex.y)
		vertex_2d = vertex_2d.rotated(vtx_angle * 0.5 * test)
		vertex = Vector3(vertex_2d.x, vertex_2d.y, 0.0)
		arrs[Mesh.ARRAY_VERTEX][i] = vertex*/
}

void ItemSelectorUI::_update_hovered_item() {
	if (showing_items.size() == 0) {
		return;
	}
	const float shortest_side_half = get_size()[get_size().min_axis_index()] * 0.5f;
	Transform2D trf;
	trf.scale(Vector2(shortest_side_half, shortest_side_half));
	trf.set_origin(get_size() * 0.5f);

	fan_mesh_instance->set_transform(trf);

	const Vector2 center = get_size() * 0.5f;
	const float mouse_rot = -((get_local_mouse_position() - center).angle_to(Vector2(1.0f, 0.0f)));
	fan_mesh_instance->rotate(mouse_rot);
	int idx = Math::floor((mouse_rot + fan_mesh.rads_per_child * 0.5f) / fan_mesh.rads_per_child);
	idx = Math::wrapi(idx, 0, item_container->get_child_count());

	if (idx == hovered_item) {
		return;
	}

	for (int i = 0; i < showing_items.size(); i++) {
		showing_items[i].icon->set_hovered(i == idx);
	}

	hovered_item = idx;

	const StringName item_name = showing_items[hovered_item].name;
	LaniakeaGameRules *game_rules = LaniakeaMainLoop::get_singleton()->get_game_rules();
	item_name_label->set_text(game_rules->item_get_localization_name(item_name));
	item_description_label->set_text(game_rules->item_get_localization_description(item_name));

	item_name_label->show();
	item_description_label->show();
}

void ItemSelectorUI::_on_selected() {
	if (hovered_item != -1) {
		if (showing_items[hovered_item].icon->get_selected()) {
			emit_signal("unequip_requested");
		} else {
			emit_signal("equip_requested", showing_items[hovered_item].name);
		}
	}

	hide_items();
}

void ItemSelectorUI::_bind_methods() {
	ADD_SIGNAL(MethodInfo("unequip_requested"));
	ADD_SIGNAL(MethodInfo("equip_requested", PropertyInfo(Variant::STRING_NAME, "item_name")));
}

void ItemSelectorUI::hide_items() {
	hide();
	if (tween.is_valid()) {
		tween->kill();
	}
	tween = create_tween();
	tween->tween_method(callable_mp(this, &ItemSelectorUI::appear_animation_progress), 0.0f, 1.0f, 0.4f)->set_ease(godot::Tween::EASE_OUT)->set_trans(godot::Tween::TRANS_BOUNCE);

	showing_items.clear();

	for (int i = item_container->get_child_count() - 1; i >= 0; i--) {
		item_container->get_child(i)->queue_free();
		item_container->remove_child(item_container->get_child(i));
	}
}

void ItemSelectorUI::show_items(Vector<StringName> p_weapon_item_names, const std::optional<StringName> p_selected) {
	hide_items();
	show();

	queue_sort();

	LaniakeaGameRules *game_rules = LaniakeaMainLoop::get_singleton()->get_game_rules();

	for (StringName item : p_weapon_item_names) {
		ItemSelectIcon *icon = instantiate_scene_type_checked<ItemSelectIcon>("res://scenes/ui/item_selector/item_select_icon.tscn");
		item_container->add_child(icon);
		icon->initialize(item);
		icon->set_selected(p_selected.has_value() && item == *p_selected);
		showing_items.push_back({
				.name = item,
				.icon = icon,
		});
	}

	const float rads_per_child = Math_TAU / static_cast<float>(item_container->get_child_count());
	if (rads_per_child != fan_mesh.rads_per_child) {
		fan_mesh.rads_per_child = rads_per_child;
		_regenerate_fan_mesh();
	}

	item_name_label->hide();
	item_description_label->hide();

	Input::get_singleton()->warp_mouse(get_global_position() + get_size() * 0.5f);
	hovered_item = -1;
	_update_hovered_item();
}

void ItemSelectorUI::_ready() {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	item_container = get_node_type_checked<RadialContainer>(this, "%ItemContainer");

	fan_mesh_instance = get_node_type_checked<MeshInstance2D>(this, "%FanMeshInstance");
	item_name_label = get_node_type_checked<Label>(this, "%ItemName");
	item_description_label = get_node_type_checked<Label>(this, "%ItemDescription");

	item_name_label->hide();
	item_description_label->hide();
}

void ItemSelectorUI::_gui_input(const Ref<InputEvent> &p_event) {
	if (Ref<InputEventMouseMotion> motion = p_event; motion.is_valid()) {
		_update_hovered_item();
	}

	if (Ref<InputEventMouseButton> button = p_event; button.is_valid()) {
		if (!button->is_echo() && button->is_pressed() && button->get_button_index() == MOUSE_BUTTON_LEFT) {
			_on_selected();
		}
	}
}

void ItemSelectorUI::_input(const Ref<InputEvent> &p_event) {
	if (Ref<InputEventKey> key = p_event; key.is_valid()) {
		if (key->is_pressed() && !key->is_echo() && key->get_key_label() == godot::KEY_F2) {
			Vector<StringName> test;
			test.push_back("weapon_gravitygun_item");
			test.push_back("weapon_rifle_test_item");
			test.push_back("weapon_gravitygun_item");
			test.push_back("weapon_rifle_test_item");
			show_items(test, "weapon_gravitygun_item");
		}
	}
}
