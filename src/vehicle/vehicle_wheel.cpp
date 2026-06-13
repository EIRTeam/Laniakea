#include "vehicle_wheel.h"

#include "bind_macros.h"
#include "godot_cpp/classes/global_constants.hpp"
#include "vehicle.h"

void LNVehicleWheel::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PARENTED: {
			if (LNVehicle *vehicle = Object::cast_to<LNVehicle>(get_parent())) {
				vehicle->register_wheel(this);
			}
		} break;
		case NOTIFICATION_UNPARENTED: {
			if (LNVehicle *vehicle = Object::cast_to<LNVehicle>(get_parent())) {
				vehicle->unregister_wheel(this);
			}
		} break;
	}
}

void LNVehicleWheel::_bind_methods() {
	MAKE_BIND_VECTOR3(LNVehicleWheel, top_attachment_point);
	MAKE_BIND_RESOURCE(LNVehicleWheel, wheel_settings, LNVehicleWheelSettings);
	MAKE_BIND_RESOURCE(LNVehicleWheel, suspension_settings, LNVehicleSuspensionSettings);
	BIND_SETTER_GETTER(LNVehicleWheel, wheel_position);
	MAKE_BIND_BOOL(LNVehicleWheel, steerable);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "wheel_position", PROPERTY_HINT_ENUM, "FL,FR,RL,RR"), "set_wheel_position", "get_wheel_position");
}
