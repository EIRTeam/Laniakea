#pragma once

#include "../bind_macros.h"
#include "godot_cpp/classes/resource.hpp"

using namespace godot;

class LNVehicleSuspensionSettings : public Resource {
    GDCLASS(LNVehicleSuspensionSettings, Resource);
    float maximum = 0.0f;
    float minimum = 0.0f;
    float rest = 0.0f;
    float natural_frequency_hz = 1.5f;
    float damping_ratio = 0.35f;

    float _spring_rate = 0.0f;
    float _damping_coefficient = 0.0f;
    bool suspension_rates_dirty = true;
    void _recalculate_suspension_rates(float p_sprung_mass);
public:
    MAKE_SETTER_GETTER_FLOAT_VALUE(maximum, maximum);
    MAKE_SETTER_GETTER_FLOAT_VALUE(minimum, minimum);
    MAKE_SETTER_GETTER_FLOAT_VALUE(rest, rest);
    MAKE_SETTER_GETTER_FLOAT_VALUE(natural_frequency_hz, natural_frequency_hz);
    MAKE_SETTER_GETTER_FLOAT_VALUE(damping_ratio, damping_ratio);

    float get_spring_rate(const float p_sprung_mass) const;
    float get_damping_coefficient(const float p_sprung_mass) const;

    static void _bind_methods();
};