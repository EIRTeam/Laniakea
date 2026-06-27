#include "vehicle_telemetry.h"

#include "godot_cpp/core/print_string.hpp"

void VehicleTelemetry::create_data_channel(StringName p_channel_name, TelemetryChannelType p_type) {
	channel_names.insert(p_channel_name);
}

HashSet<StringName> VehicleTelemetry::get_telemetry_channel_names() const {
	return channel_names;
}

VehicleTelemetry::TelemetryChannelType VehicleTelemetry::get_telemetry_channel_type(StringName p_channel_name) const {
	auto channel_it = channels.find(p_channel_name);
	DEV_ASSERT(channel_it != channels.end());
	DEV_ASSERT(channel_it->value->type == TelemetryChannelType::LINE_GRAPH);
	TelemetryLineGraphChannel *channel = dynamic_cast<TelemetryLineGraphChannel *>(channel_it->value);
	return channel->type;
}

void VehicleTelemetry::create_line_graph_data_channel(StringName p_channel_name, float p_min, float p_max, int p_telemetry_points, Span<LineGraphSubchannelCreateInfo> p_subchannel_create_info) {
	channel_names.insert(p_channel_name);
	TelemetryLineGraphChannel *channel = memnew(TelemetryLineGraphChannel);

	channel->type = TelemetryChannelType::LINE_GRAPH;
	channel->telemetry_points = p_telemetry_points;
	channel->min = p_min;
	channel->max = p_max;

	for (const LineGraphSubchannelCreateInfo &create_info : p_subchannel_create_info) {
		auto subchannel = channel->subchannels.insert(create_info.name, {
																				.display_name = create_info.display_name.is_empty() ? String(create_info.name.capitalize()) : String(create_info.display_name),
																				.color = create_info.color,
																		});
		subchannel->value.values.resize(p_telemetry_points);
	}

	channels.insert(p_channel_name, channel);
}

void VehicleTelemetry::push_line_graph_data_channel_update(StringName p_channel_name, StringName p_subchannel_name, float p_value) {
	auto channel_it = channels.find(p_channel_name);
	DEV_ASSERT(channel_it != channels.end());
	DEV_ASSERT(channel_it->value->type == TelemetryChannelType::LINE_GRAPH);
	TelemetryLineGraphChannel *channel = dynamic_cast<TelemetryLineGraphChannel *>(channel_it->value);
	auto subchannel_it = channel->subchannels.find(p_subchannel_name);
	DEV_ASSERT(subchannel_it != channel->subchannels.end());

	for (int i = 0; i < subchannel_it->value.values.size() - 1; i++) {
		subchannel_it->value.values[i] = subchannel_it->value.values[i + 1];
	}

	subchannel_it->value.values[subchannel_it->value.values.size() - 1] = p_value;
}

int VehicleTelemetry::line_graph_get_channel_get_telemetry_point_count(StringName p_channel_name) const {
	auto channel_it = channels.find(p_channel_name);
	DEV_ASSERT(channel_it != channels.end());
	DEV_ASSERT(channel_it->value->type == TelemetryChannelType::LINE_GRAPH);
	TelemetryLineGraphChannel *channel = dynamic_cast<TelemetryLineGraphChannel *>(channel_it->value);
	return channel->telemetry_points;
}

String VehicleTelemetry::get_line_graph_subchannel_display_name(StringName p_channel_name, StringName p_subchannel_name) const {
	auto channel_it = channels.find(p_channel_name);
	DEV_ASSERT(channel_it != channels.end());
	DEV_ASSERT(channel_it->value->type == TelemetryChannelType::LINE_GRAPH);
	TelemetryLineGraphChannel *channel = dynamic_cast<TelemetryLineGraphChannel *>(channel_it->value);
	auto subchannel_it = channel->subchannels.find(p_subchannel_name);
	DEV_ASSERT(subchannel_it != channel->subchannels.end());
	return subchannel_it->value.display_name;
}

Span<float> VehicleTelemetry::get_line_graph_subchannel_telemetry_points(StringName p_channel_name, StringName p_subchannel_name) const {
	auto channel_it = channels.find(p_channel_name);
	DEV_ASSERT(channel_it != channels.end());
	DEV_ASSERT(channel_it->value->type == TelemetryChannelType::LINE_GRAPH);
	TelemetryLineGraphChannel *channel = dynamic_cast<TelemetryLineGraphChannel *>(channel_it->value);
	auto subchannel_it = channel->subchannels.find(p_subchannel_name);
	DEV_ASSERT(subchannel_it != channel->subchannels.end());
	return subchannel_it->value.values;
}

Vector<StringName> VehicleTelemetry::get_line_graph_subchannels(StringName p_channel_name) const {
	auto channel_it = channels.find(p_channel_name);
	DEV_ASSERT(channel_it != channels.end());
	DEV_ASSERT(channel_it->value->type == TelemetryChannelType::LINE_GRAPH);
	TelemetryLineGraphChannel *channel = dynamic_cast<TelemetryLineGraphChannel *>(channel_it->value);

	Vector<StringName> subchannels;

	for (const auto &it : channel->subchannels) {
		subchannels.push_back(it.key);
	}

	return subchannels;
}

float VehicleTelemetry::get_telemetry_channel_max(StringName p_channel_name) const {
	auto channel_it = channels.find(p_channel_name);
	DEV_ASSERT(channel_it != channels.end());
	DEV_ASSERT(channel_it->value->type == TelemetryChannelType::LINE_GRAPH);
	TelemetryLineGraphChannel *channel = dynamic_cast<TelemetryLineGraphChannel *>(channel_it->value);
	return channel->max;
}

float VehicleTelemetry::get_telemetry_channel_min(StringName p_channel_name) const {
	auto channel_it = channels.find(p_channel_name);
	DEV_ASSERT(channel_it != channels.end());
	DEV_ASSERT(channel_it->value->type == TelemetryChannelType::LINE_GRAPH);
	TelemetryLineGraphChannel *channel = dynamic_cast<TelemetryLineGraphChannel *>(channel_it->value);
	return channel->min;
}

std::optional<Color> VehicleTelemetry::get_line_graph_subchannel_color(StringName p_channel_name, StringName p_subchannel_name) const {
	auto channel_it = channels.find(p_channel_name);
	DEV_ASSERT(channel_it != channels.end());
	DEV_ASSERT(channel_it->value->type == TelemetryChannelType::LINE_GRAPH);
	TelemetryLineGraphChannel *channel = dynamic_cast<TelemetryLineGraphChannel *>(channel_it->value);
	auto subchannel_it = channel->subchannels.find(p_subchannel_name);
	DEV_ASSERT(subchannel_it != channel->subchannels.end());
	return subchannel_it->value.color;
}

VehicleTelemetry::~VehicleTelemetry() {
	for (auto it : channels) {
		memdelete(it.value);
	}
}
