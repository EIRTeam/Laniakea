#pragma once

#include "chunkinator/chunkinator.h"
#include "godot_cpp/classes/control.hpp"
#include "godot_cpp/classes/input_event.hpp"
#include "godot_cpp/classes/item_list.hpp"
#include "godot_cpp/classes/panel_container.hpp"
#include "godot_cpp/variant/transform2d.hpp"

using namespace godot;

class ChunkinatorDebugLayerViewer : public Control {
	GDCLASS(ChunkinatorDebugLayerViewer, Control);
	float view_radius = 30000;
	Vector2 offset;
	bool has_snapshot = false;
	ChunkinatorLayerDebugSnapshot layer_snapshot;
	Transform2D draw_trf_cache;
	bool draw_trf_dirty = true;
	Rect2i generation_rect;

private:
	Transform2D get_draw_trf() const;
	void _draw_debug_element(const ChunkinatorDebugDrawer::DrawTextureRect &p_draw_command);
	void _draw_debug_element(const ChunkinatorDebugDrawer::DrawCircle &p_draw_command);
	void _draw_debug_element(const ChunkinatorDebugDrawer::DrawLine &p_draw_command);
	void _draw_debug_element(const ChunkinatorDebugDrawer::DrawRect &p_draw_command);

public:
	void set_layer_snapshot(ChunkinatorLayerDebugSnapshot p_debug_snapshot);
	void set_generation_rect(Rect2i p_generation_rect);
	void _notification(int p_what);
	virtual void _gui_input(const Ref<InputEvent> &p_event) override;
	static void _bind_methods();
	ChunkinatorDebugLayerViewer();
};

class ChunkinatorDebugSnapshotViewer : public PanelContainer {
	Ref<ChunkinatorDebugSnapshot> debug_snapshot;
	ChunkinatorDebugLayerViewer *layer_viewer = nullptr;
	ItemList *layer_list = nullptr;

private:
	void _on_layer_selected(int p_idx);
	void _populate_layer_list();

public:
	ChunkinatorDebugSnapshotViewer(const Ref<ChunkinatorDebugSnapshot> &p_debug_snapshot);
};
