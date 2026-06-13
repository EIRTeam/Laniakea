#pragma once

#include "godot_cpp/classes/panel_container.hpp"

using namespace godot;

class LNVehicle;

namespace godot {
class GraphEdit;
class GraphNode;
class Label;
class HSeparator;
} //namespace godot

class LNVehicleShaft;

class LNVehicleDrivetrainDebugger : public PanelContainer {
	GDCLASS(LNVehicleDrivetrainDebugger, PanelContainer);

	GraphEdit *graph_edit = nullptr;

	struct PerShaftNode {
		Label *input_label = nullptr;
		Label *debug_label = nullptr;
		Vector<Label *> output_labels;
		GraphNode *graph_node;
		HSeparator *separator;
		Ref<LNVehicleShaft> shaft;
	};
	HashMap<StringName, PerShaftNode> per_shaft_nodes;

public:
	void update();
	void update_tree(LNVehicle *p_vehicle);
	virtual void _ready() override;
	static void _bind_methods() {}
};
