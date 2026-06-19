#pragma once

#include "godot_cpp/templates/hash_map.hpp"
#include "godot_cpp/templates/hash_set.hpp"

#include <optional>
#include <variant>

using namespace godot;

class VehicleTelemetry {
public:
	enum class TelemetryChannelType {
		LINE_GRAPH
	};

private:
	struct TelemetryLineGraphSubChannel {
		StringName display_name;
		std::optional<Color> color;
		LocalVector<float> values;
	};

	struct TelemetryChannel {
		StringName name;
		TelemetryChannelType type;
		virtual ~TelemetryChannel() = default;
	};

	struct TelemetryLineGraphChannel : public TelemetryChannel {
		HashMap<StringName, TelemetryLineGraphSubChannel> subchannels;
		float min;
		float max;
		int telemetry_points = 0;
	};

	HashMap<StringName, TelemetryChannel *> channels;
	HashSet<StringName> channel_names;

public:
	void create_data_channel(StringName p_category_name, TelemetryChannelType p_type);
	struct LineGraphSubchannelCreateInfo {
		StringName name;
		StringName display_name;
		std::optional<Color> color;
	};
	void create_line_graph_data_channel(StringName p_channel_name, float p_min, float p_max, int p_telemetry_points, Span<LineGraphSubchannelCreateInfo> p_subchannel_create_info);
	void push_line_graph_data_channel_update(StringName p_channel_name, StringName p_subchannel_update, float p_value);
	int line_graph_get_channel_get_telemetry_point_count(StringName p_channel_name) const;
	Vector<StringName> get_line_graph_subchannels(StringName p_channel_name) const;
	Span<float> get_line_graph_subchannel_telemetry_points(StringName p_channel_name, StringName p_subchannel_name) const;
	std::optional<Color> get_line_graph_subchannel_color(StringName p_channel_name, StringName p_subchannel_name) const;
	String get_line_graph_subchannel_display_name(StringName p_channel_name, StringName p_subchannel_name) const;
	HashSet<StringName> get_telemetry_channel_names() const;
	TelemetryChannelType get_telemetry_channel_type(StringName p_channel_name) const;
	float get_telemetry_channel_max(StringName p_channel_name) const;
	float get_telemetry_channel_min(StringName p_channel_name) const;

	~VehicleTelemetry();
};
