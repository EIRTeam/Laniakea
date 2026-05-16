#pragma once

#include "godot_cpp/classes/resource.hpp"
#include "../bind_macros.h"

using namespace godot;

class LNVehicleWheelSettings : public Resource {
    GDCLASS(LNVehicleWheelSettings, Resource);
    
    float radius = 0.283813f;
    float width = 0.165f;
    float mass = 20.0f;
public:
    MAKE_SETTER_GETTER_VALUE(float, radius, radius);
    MAKE_SETTER_GETTER_VALUE(float, width, width);
    MAKE_SETTER_GETTER_VALUE(float, mass, mass);

    static void _bind_methods();
};