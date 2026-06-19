#pragma once

#include "godot_cpp/classes/item_list.hpp"
#include "godot_cpp/classes/panel_container.hpp"
#include "godot_cpp/classes/scroll_container.hpp"
#include "godot_cpp/classes/tree.hpp"
#include "godot_cpp/classes/v_box_container.hpp"
#include "godot_cpp/classes/window.hpp"
#include "vehicle/telemetry/vehicle_telemetry_drawer.h"

using namespace godot;

class VehicleTelemetry;

class VehicleTelemetryControl : public Control {
	GDCLASS(VehicleTelemetryControl, Control);

	struct CategoryTreeItem {
		TreeItem *item = nullptr;
		VehicleTelemetryDrawer *drawer = nullptr;
	};

	HashMap<StringName, CategoryTreeItem> category_nodes;

	Tree *category_list = nullptr;
	ScrollContainer *telemetry_values_scroll_container = nullptr;
	VBoxContainer *telemetry_values_vbox = nullptr;
	VehicleTelemetry *telemetry = nullptr;

private:
	void _on_item_edited();

public:
	void update(VehicleTelemetry *p_telemetry);
	bool is_channel_enabled(StringName p_channel_name) const;
	virtual void _ready() override;
	void _update_categories(VehicleTelemetry *p_telemetry);
	static void _bind_methods() {}
};
