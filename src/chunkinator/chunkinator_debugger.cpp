#include "chunkinator_debugger.h"

#include "chunkinator/chunkinator_debug_snapshot_viewer.h"
#include "gdextension_interface.h"
#include "godot_cpp/classes/graph_node.hpp"
#include "godot_cpp/classes/v_split_container.hpp"

void ChunkinatorDebugger::_on_debug_snapshot_created(Ref<ChunkinatorDebugSnapshot> p_debug_snapshot) {
	const int item_idx = snapshot_list->add_item(vformat("Snapshot (%d layers)", p_debug_snapshot->get_per_layer_debug_data().size()));
	snapshot_list->set_item_metadata(item_idx, p_debug_snapshot);
}

void ChunkinatorDebugger::_update_graph_tab() {
	layer_graph_nodes.clear();
	for (int i = 0; i < layer_graph_nodes.size(); i++) {
		layer_graph_nodes[i]->queue_free();
	}

	layer_graph_nodes.reserve(chunkinator->layers.size());

	for (int i = 0; i < chunkinator->layers.size(); i++) {
		GraphNode *node = memnew(GraphNode);
		Ref<ChunkinatorLayer> layer = chunkinator->layers[i];
		node->set_title(layer->get_name());

		Control *c = memnew(Control);
		node->add_child(c);
		node->set_slot(0, !layer->parents.is_empty(), GDEXTENSION_VARIANT_TYPE_NIL, Color(1.0, 1.0, 1.0), !layer->children.is_empty(), GDEXTENSION_VARIANT_TYPE_NIL, Color(1.0, 1.0, 1.0));
		node->set_name(layer->get_name());

		graph_tab->add_child(node);
		layer_graph_nodes.push_back(node);
	}

	for (int i = 0; i < chunkinator->layers.size(); i++) {
		for (int parent_i = 0; parent_i < chunkinator->layers[i]->parents.size(); parent_i++) {
			StringName parent_name = chunkinator->layers[i]->parents[parent_i];
			StringName child_name = chunkinator->layers[i]->get_name();

			graph_tab->connect_node(parent_name, 0, child_name, 0);
		}
	}

	graph_tab->arrange_nodes();
}

void ChunkinatorDebugger::set_chunkinator(Ref<Chunkinator> p_chunkinator) {
	chunkinator = p_chunkinator;
	p_chunkinator->connect("debug_snapshot_created", callable_mp(this, &ChunkinatorDebugger::_on_debug_snapshot_created));
	_update_graph_tab();
}

void ChunkinatorDebugger::_on_snapshot_selected(int p_idx) {
	Ref<ChunkinatorDebugSnapshot> debug_snapshot = snapshot_list->get_item_metadata(p_idx);
	ChunkinatorDebugSnapshotViewer *viewer = memnew(ChunkinatorDebugSnapshotViewer(debug_snapshot));
	tabs->add_child(viewer);
	tabs->set_tab_title(tabs->get_tab_count() - 1, vformat("Snapshot %d", p_idx));
}

void ChunkinatorDebugger::_bind_methods() {
}

ChunkinatorDebugger::ChunkinatorDebugger() {
	tabs = memnew(TabContainer);
	add_child(tabs);
	tabs->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);

	snapshot_list = memnew(ItemList);
	tabs->add_child(snapshot_list);
	tabs->set_tab_title(tabs->get_tab_count() - 1, "Snapshots");
	snapshot_list->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	snapshot_list->connect("item_activated", callable_mp(this, &ChunkinatorDebugger::_on_snapshot_selected));

	graph_tab = memnew(GraphEdit);
	tabs->add_child(graph_tab);
	tabs->set_tab_title(tabs->get_tab_count() - 1, "Graph");
}
