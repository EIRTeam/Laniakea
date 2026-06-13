#pragma once

#include "chunkinator/chunkinator.h"
#include "godot_cpp/classes/control.hpp"
#include "godot_cpp/classes/graph_edit.hpp"
#include "godot_cpp/classes/graph_node.hpp"
#include "godot_cpp/classes/item_list.hpp"
#include "godot_cpp/classes/panel_container.hpp"
#include "godot_cpp/classes/tab_container.hpp"
#include "godot_cpp/templates/hash_map.hpp"

using namespace godot;

class ChunkinatorLayer;

class ChunkinatorDebugger : public Control {
	GDCLASS(ChunkinatorDebugger, Control);

	LocalVector<Ref<ChunkinatorDebugSnapshot>> debug_snapshots;

	void _on_debug_snapshot_created(Ref<ChunkinatorDebugSnapshot> p_debug_snapshot);
	TabContainer *tabs = nullptr;
	ItemList *snapshot_list;
	GraphEdit *graph_tab;
	Ref<Chunkinator> chunkinator;
	LocalVector<GraphNode *> layer_graph_nodes;
	void _update_graph_tab();

public:
	void set_chunkinator(Ref<Chunkinator> p_chunkinator);
	void _on_snapshot_selected(int p_idx);
	static void _bind_methods();
	ChunkinatorDebugger();
};
