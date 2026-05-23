#pragma once

#include "godot_cpp/classes/resource.hpp"
#include "../bind_macros.h"

using namespace godot;

class LNVehicleWheelSettings : public Resource {
    GDCLASS(LNVehicleWheelSettings, Resource);
    
    float radius = 0.283813f;
    float width = 0.165f;
    float mass = 20.0f;

    float coefficient_of_friction = 0.95f;
    float stiffness = 5.5f;
    float contact_patch = 0.2f;
public:
    MAKE_SETTER_GETTER_VALUE(float, radius, radius);
    MAKE_SETTER_GETTER_VALUE(float, width, width);
    MAKE_SETTER_GETTER_VALUE(float, mass, mass);
    MAKE_SETTER_GETTER_VALUE(float, coefficient_of_friction, coefficient_of_friction);
    MAKE_SETTER_GETTER_VALUE(float, contact_patch, contact_patch);
    MAKE_SETTER_GETTER_VALUE(float, stiffness, stiffness);

    static void _bind_methods();
};