#include "vehicle_telemetry_window.h"

#include "godot_cpp/classes/h_split_container.hpp"
#include "godot_cpp/classes/scroll_container.hpp"
#include "godot_cpp/classes/tree_item.hpp"
#include "godot_cpp/classes/v_box_container.hpp"
#include "godot_cpp/classes/v_split_container.hpp"
#include "godot_cpp/variant/packed_string_array.hpp"
#include "godot_cpp/variant/string_name.hpp"
#include "vehicle/telemetry/vehicle_telemetry.h"
#include "vehicle/telemetry/vehicle_telemetry_drawer_line_graph.h"

void VehicleTelemetryControl::_ready() {
	telemetry_values_vbox = memnew(VBoxContainer);
	HSplitContainer *hsplit = memnew(HSplitContainer);
	hsplit->set_anchors_and_offsets_preset(Control::LayoutPreset::PRESET_FULL_RECT);
	hsplit->set_split_offset(150);
	add_child(hsplit);

	telemetry_values_scroll_container = memnew(ScrollContainer);

	category_list = memnew(Tree);
	category_list->set_hide_root(true);
	category_list->set_columns(1);
	hsplit->add_child(category_list);
	hsplit->add_child(telemetry_values_scroll_container);

	telemetry_values_vbox->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
	telemetry_values_vbox->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);

	telemetry_values_scroll_container->add_child(telemetry_values_vbox);

	category_list->clear();

	category_list->connect("item_edited", callable_mp(this, &VehicleTelemetryControl::_on_item_edited));
}

void VehicleTelemetryControl::_on_item_edited() {
	TreeItem *edited_item = category_list->get_edited();
	const StringName channel_name = edited_item->get_meta("channel");
	auto channel_node_it = category_nodes.find(channel_name);
	DEV_ASSERT(channel_node_it != category_nodes.end());
	const bool checked = edited_item->is_checked(0);
	if (!checked && channel_node_it->value.drawer != nullptr) {
		memdelete(channel_node_it->value.drawer);
		channel_node_it->value.drawer = nullptr;
	} else if (checked) {
		VehicleTelemetryDrawer *drawer = nullptr;
		switch (telemetry->get_telemetry_channel_type(channel_name)) {
			case VehicleTelemetry::TelemetryChannelType::LINE_GRAPH: {
				drawer = memnew(VehicleTelemetryDrawerLineGraph);
			} break;
		}
		telemetry_values_vbox->add_child(drawer);
		channel_node_it->value.drawer = drawer;
		drawer->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
	}
}

void VehicleTelemetryControl::_update_categories(VehicleTelemetry *p_telemetry) {
	telemetry = p_telemetry;
	HashSet<StringName> channel_names = p_telemetry->get_telemetry_channel_names();

	category_nodes.clear();
	category_list->clear();

	TreeItem *root = category_list->create_item();

	for (const StringName &channel : channel_names) {
		PackedStringArray parts = channel.split("/");
		if (parts.is_empty()) {
			continue;
		}
		StringName channel_accum;

		TreeItem *tree_item = root;

		channel_accum = parts[0];
		for (int i = 0; i < parts.size(); i++) {
			if (i != 0) {
				channel_accum = String(channel_accum) + "/" + parts[i];
			}
			auto it = category_nodes.find(channel_accum);
			if (it == category_nodes.end()) {
				TreeItem *new_item = category_list->create_item(tree_item);
				new_item->set_text(0, parts[i].capitalize());
				new_item->set_meta("channel", channel_accum);
				it = category_nodes.insert(channel_accum, { .item = new_item });
			}

			tree_item = it->value.item;
		}
		tree_item->set_cell_mode(0, TreeItem::CELL_MODE_CHECK);
		tree_item->set_editable(0, true);
		tree_item->set_text(0, parts[parts.size() - 1].capitalize());
		tree_item->set_checked(0, is_channel_enabled(channel));
	}
}

void VehicleTelemetryControl::update(VehicleTelemetry *p_telemetry) {
	if (category_nodes.is_empty()) {
		_update_categories(p_telemetry);
	}

	for (auto kv : category_nodes) {
		if (kv.value.drawer != nullptr) {
			kv.value.drawer->update(p_telemetry, kv.key);
		}
	}
}

bool VehicleTelemetryControl::is_channel_enabled(StringName p_channel_name) const {
	return false;
}
