#pragma once

#include "../bind_macros.h"
#include "godot_cpp/classes/resource.hpp"
#include "vehicle/vehicle_tyre.h"

using namespace godot;

class LNVehicleWheelSettings : public Resource {
	GDCLASS(LNVehicleWheelSettings, Resource);

	float mass = 20.0f;
	Ref<LNVehicleTyre> tyre;

	/*float coefficient_of_friction = 0.95f;
	float stiffness = 5.5f;
	float contact_patch = 0.2f;
	*/

public:
	MAKE_SETTER_GETTER_VALUE(float, mass, mass);
	MAKE_SETTER_GETTER_VALUE(Ref<LNVehicleTyre>, tyre, tyre);

	static void _bind_methods();
};
