#pragma once

#include "godot_cpp/classes/control.hpp"
#include "godot_cpp/classes/h_box_container.hpp"
#include "vehicle/telemetry/vehicle_telemetry.h"

using namespace godot;

class VehicleTelemetryDrawer : public BoxContainer {
	GDCLASS(VehicleTelemetryDrawer, BoxContainer);

public:
	void _notification(int p_what);
	virtual void update(VehicleTelemetry *p_telemetry, StringName p_channel) = 0;
	static void _bind_methods() {}
};
