#pragma once

#include "../bind_macros.h"
#include "godot_cpp/classes/resource.hpp"

using namespace godot;

class LNVehicleSuspensionSettings : public Resource {
    GDCLASS(LNVehicleSuspensionSettings, Resource);
    float minimum = 0.0f;
    float maximum = 0.0f;
    float rest = 0.0f;
    
    float spring_rate = 4250.0f;
    float bump = 0.3f;
    float rebound = 0.7f;
public:
    MAKE_SETTER_GETTER_FLOAT_VALUE(maximum, maximum);
    MAKE_SETTER_GETTER_FLOAT_VALUE(minimum, minimum);
    MAKE_SETTER_GETTER_FLOAT_VALUE(rest, rest);
    MAKE_SETTER_GETTER_FLOAT_VALUE(bump, bump);
    MAKE_SETTER_GETTER_FLOAT_VALUE(rebound, rebound);
    MAKE_SETTER_GETTER_FLOAT_VALUE(spring_rate, spring_rate);

    static void _bind_methods();
};