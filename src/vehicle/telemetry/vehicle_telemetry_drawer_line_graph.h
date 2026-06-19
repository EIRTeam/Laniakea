#pragma once

#include "godot_cpp/classes/h_box_container.hpp"
#include "godot_cpp/classes/label.hpp"
#include "godot_cpp/classes/panel_container.hpp"
#include "godot_cpp/classes/v_box_container.hpp"
#include "godot_cpp/variant/packed_vector2_array.hpp"
#include "vehicle/telemetry/vehicle_telemetry_drawer.h"
class VehicleTelemetryDrawerLineGraph : public VehicleTelemetryDrawer {
	GDCLASS(VehicleTelemetryDrawerLineGraph, VehicleTelemetryDrawer);

	struct SubchannelDrawData {
		bool enabled = true;
		StringName name;
		String display_name;
		Color color;
		Vector<float> values;
		PackedVector2Array draw_points;
	};

	Control *graph_container = nullptr;
	VBoxContainer *legend_container = nullptr;
	Label *title_label = nullptr;

	std::array<Color, 8> auto_palette = {
		Color("#5f5fd9"),
		Color("#a254d2"),
		Color("#d446be"),
		Color("#f83ea2"),
		Color("#ff4880"),
		Color("#ff635d"),
		Color("#ff8439"),
		Color("#ffa600"),
	};

	LocalVector<SubchannelDrawData> subchannel_datas;

	float min_y = 0.0f;
	float max_y = 0.0f;

	void _add_legend(const SubchannelDrawData &p_draw_data);
	void _on_subchannel_toggled(bool p_toggled_on, const int p_subchannel_idx);

public:
	void _notification(int p_what);
	virtual void update(VehicleTelemetry *p_telemetry, StringName p_channel) override;
	static void _bind_methods() {}
};
