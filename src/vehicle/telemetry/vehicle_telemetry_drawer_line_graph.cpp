#include "vehicle_telemetry_drawer_line_graph.h"

#include "godot_cpp/classes/canvas_item.hpp"
#include "godot_cpp/classes/check_box.hpp"
#include "godot_cpp/classes/color_rect.hpp"
#include "godot_cpp/classes/control.hpp"
#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/classes/h_box_container.hpp"
#include "godot_cpp/classes/label.hpp"
#include "godot_cpp/classes/panel_container.hpp"
#include "godot_cpp/classes/style_box.hpp"
#include "godot_cpp/classes/v_box_container.hpp"
#include "godot_cpp/core/math.hpp"

#include <algorithm>

void VehicleTelemetryDrawerLineGraph::update(VehicleTelemetry *p_telemetry, StringName p_channel) {
	queue_redraw();
	Vector<StringName> subchannels = p_telemetry->get_line_graph_subchannels(p_channel);
	min_y = p_telemetry->get_telemetry_channel_min(p_channel);
	max_y = p_telemetry->get_telemetry_channel_max(p_channel);

	if (title_label->get_text().is_empty()) {
		title_label->set_text(p_channel);
	}

	for (const StringName &subchannel : subchannels) {
		auto it = std::find_if(subchannel_datas.begin(), subchannel_datas.end(), [subchannel](const SubchannelDrawData &p_draw_data) {
			return p_draw_data.name == subchannel;
		});

		SubchannelDrawData *draw_data;

		if (it == subchannel_datas.end()) {
			subchannel_datas.push_back({
					.name = subchannel,
					.display_name = p_telemetry->get_line_graph_subchannel_display_name(p_channel, subchannel),
					.color = p_telemetry->get_line_graph_subchannel_color(p_channel, subchannel).value_or(auto_palette[subchannel_datas.size() % auto_palette.size()]),
			});
			draw_data = &subchannel_datas[subchannel_datas.size() - 1];
			draw_data->values.resize(p_telemetry->line_graph_get_channel_get_telemetry_point_count(p_channel));
			draw_data->draw_points.resize(draw_data->values.size());
			_add_legend(*draw_data);
		} else {
			draw_data = &*it;
		}
		Span<float> telemetry_points = p_telemetry->get_line_graph_subchannel_telemetry_points(p_channel, subchannel);
		float *data_ptrw = draw_data->values.ptrw();
		for (int i = 0; i < telemetry_points.size(); i++) {
			data_ptrw[i] = telemetry_points[i];
		}
	}
}

void VehicleTelemetryDrawerLineGraph::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			set_custom_minimum_size(Vector2(300, 300));

			VBoxContainer *main_vbox_container = memnew(VBoxContainer);
			add_child(main_vbox_container);
			main_vbox_container->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
			main_vbox_container->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);

			title_label = memnew(Label);
			title_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
			main_vbox_container->add_child(title_label);
			title_label->set_h_size_flags(SIZE_EXPAND_FILL);

			HBoxContainer *main_hbox_container = memnew(HBoxContainer);
			main_hbox_container->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
			main_hbox_container->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
			main_vbox_container->add_child(main_hbox_container);

			graph_container = memnew(Control);
			main_hbox_container->add_child(graph_container);

			graph_container->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);

			const float margin = get_theme_constant("margin", "VehicleTelemetryDrawer");
			const float legend_margin = get_theme_constant("legend_margin", "VehicleTelemetryDrawerLineGraph");
			legend_container = memnew(VBoxContainer);
			main_hbox_container->add_child(legend_container);
		} break;
		case NOTIFICATION_DRAW: {
			const float margin = get_theme_constant("margin", "VehicleTelemetryDrawer");

			Rect2 graph_rect = graph_container->get_rect();
			graph_rect.grow_by(-margin);

			draw_line(graph_rect.position + Vector2(0.0f, graph_rect.size.y * 0.5f), graph_rect.position + graph_rect.size * Vector2(1.0f, 0.5f), Color("White").darkened(0.5f));

			for (SubchannelDrawData &draw_data : subchannel_datas) {
				if (!draw_data.enabled) {
					continue;
				}
				Vector2 *draw_pos_ptrw = draw_data.draw_points.ptrw();
				DEV_ASSERT(draw_data.draw_points.size() == draw_data.values.size());
				for (int64_t i = 0; i < draw_data.values.size(); i++) {
					draw_pos_ptrw[i].x = (i / static_cast<float>(draw_data.values.size() - 1)) * graph_rect.size.x;
					draw_pos_ptrw[i].y = (1.0f - Math::inverse_lerp(min_y, max_y, draw_data.values[i])) * graph_rect.size.y;
					draw_pos_ptrw[i] += graph_rect.position;
				}

				draw_polyline(draw_data.draw_points, draw_data.color, 2.0f, true);
			}
			draw_line(graph_rect.position, graph_rect.position + Vector2(0.0f, graph_rect.size.y), Color("White"), 2.0f);
			draw_line(graph_rect.position + Vector2(0.0f, graph_rect.size.y), graph_rect.position + graph_rect.size, Color("White"), 2.0f);
		} break;
	}
}

void VehicleTelemetryDrawerLineGraph::_add_legend(const SubchannelDrawData &p_draw_data) {
	HBoxContainer *hbox = memnew(HBoxContainer);

	ColorRect *color_rect = memnew(ColorRect);
	color_rect->set_custom_minimum_size(Vector2(24, 24));
	color_rect->set_color(p_draw_data.color);

	CheckBox *checkbox = memnew(CheckBox);
	checkbox->set_text(p_draw_data.display_name);
	checkbox->set_pressed(p_draw_data.enabled);
	checkbox->connect("toggled", callable_mp(this, &VehicleTelemetryDrawerLineGraph::_on_subchannel_toggled).bind(subchannel_datas.size() - 1));

	hbox->add_child(color_rect);
	hbox->add_child(checkbox);

	legend_container->add_child(hbox);
}

void VehicleTelemetryDrawerLineGraph::_on_subchannel_toggled(bool p_toggled_on, const int p_subchannel_idx) {
	ERR_FAIL_INDEX(p_subchannel_idx, subchannel_datas.size());
	subchannel_datas[p_subchannel_idx].enabled = p_toggled_on;
	queue_redraw();
}
