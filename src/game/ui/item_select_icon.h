#pragma once

#include "godot_cpp/classes/panel_container.hpp"

using namespace godot;

namespace godot {
class TextureRect;
class Label;
} //namespace godot

class ItemSelectIcon : public PanelContainer {
	GDCLASS(ItemSelectIcon, PanelContainer);

	static void _bind_methods();

	TextureRect *texture_rect = nullptr;
	Label *name_label = nullptr;
	bool hovered = false;
	bool selected = false;

public:
	void set_selected(bool p_selected);
	bool get_selected() const;
	virtual void _ready() override;
	void initialize(StringName p_item_name);
	void set_hovered(bool p_hovered);
	void _update_stylebox();
};
