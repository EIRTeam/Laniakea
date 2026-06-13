#pragma once

#include "godot_cpp/classes/mesh.hpp"
#include "godot_cpp/classes/panel_container.hpp"

#include <optional>

using namespace godot;

namespace godot {
class MeshInstance2D;
class Label;
} //namespace godot

class RadialContainer;
class ItemSelectIcon;

class ItemSelectorUI : public PanelContainer {
	GDCLASS(ItemSelectorUI, PanelContainer);

	struct ShowingItem {
		StringName name;
		ItemSelectIcon *icon = nullptr;
	};

	LocalVector<ShowingItem> showing_items;

	void appear_animation_progress(float p_progress);

	Ref<Tween> tween;

	struct SelectFanMesh {
		Ref<Mesh> mesh;
		float rads_per_child = 0.0f;
	} fan_mesh;

	RadialContainer *item_container = nullptr;
	MeshInstance2D *fan_mesh_instance = nullptr;
	Label *item_name_label = nullptr;
	Label *item_description_label = nullptr;

	void _regenerate_fan_mesh();
	int hovered_item = -1;

	void _update_hovered_item();

public:
	void _on_selected();
	static void _bind_methods();
	void hide_items();
	void show_items(Vector<StringName> p_weapon_item_names, const std::optional<StringName> p_selected);
	virtual void _ready() override;
	virtual void _gui_input(const Ref<InputEvent> &p_event) override;
	virtual void _input(const Ref<InputEvent> &p_event) override;
};
