#pragma once

#include "bind_macros.h"
#include "godot_cpp/classes/resource.hpp"

using namespace godot;

class LNVehicleTyre : public Resource {
	GDCLASS(LNVehicleTyre, Resource);

public:
	float radius = 0.3f;
	float width = 0.165f;

	struct ForcesResult {
		float lateral = 0.0f;
		float longitudinal = 0.0f;
		float self_centering_torque = 0.0f;
	};

	MAKE_SETTER_GETTER_VALUE(float, radius, radius);
	MAKE_SETTER_GETTER_VALUE(float, width, width);

	virtual ForcesResult forces(float p_sx, float p_sy, float p_vertical_load) const = 0;

	static void _bind_methods() {
		MAKE_BIND_FLOAT(LNVehicleTyre, radius);
		MAKE_BIND_FLOAT(LNVehicleTyre, width);
	}
};
