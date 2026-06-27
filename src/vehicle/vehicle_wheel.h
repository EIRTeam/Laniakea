#pragma once

#include "../bind_macros.h"
#include "godot_cpp/classes/node3d.hpp"
#include "godot_cpp/classes/ref_counted.hpp"
#include "vehicle.h"
#include "vehicle/vehicle_tyre.h"
#include "vehicle_suspension_settings.h"
#include "vehicle_wheel_settings.h"

using namespace godot;

class LNVehicleWheel : public Node3D {
	GDCLASS(LNVehicleWheel, Node3D);
	Ref<LNVehicleWheelSettings> wheel_settings;
	Ref<LNVehicleSuspensionSettings> suspension_settings;
	Ref<LNVehicleTyre> tyre;
	Vector3 top_attachment_point;
	LNVehicleWheelPosition wheel_position = LNVehicleWheelPosition::WHEEL_FL;
	bool steerable = false;

public:
	MAKE_SETTER_GETTER_VALUE(Ref<LNVehicleWheelSettings>, wheel_settings, wheel_settings);
	MAKE_SETTER_GETTER_VALUE(Ref<LNVehicleTyre>, tyre, tyre);
	MAKE_SETTER_GETTER_VALUE(bool, steerable, steerable);
	MAKE_SETTER_GETTER_VALUE(Ref<LNVehicleSuspensionSettings>, suspension_settings, suspension_settings);
	MAKE_SETTER_GETTER_VALUE(Vector3, top_attachment_point, top_attachment_point);
	MAKE_SETTER_GETTER_VALUE(LNVehicleWheelPosition, wheel_position, wheel_position);

	void _notification(int p_what);

	static void _bind_methods();
};
