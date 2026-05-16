#include "vehicle_suspension_settings.h"
#include "bind_macros.h"

void LNVehicleSuspensionSettings::_recalculate_suspension_rates(float p_sprung_mass) {
    const float omega_n = 2.0f * Math_PI * get_natural_frequency_hz();
    const float spring_rate = omega_n * omega_n * p_sprung_mass;
    const float damping_coeff = 2.0f * get_damping_ratio() * omega_n * p_sprung_mass;
    suspension_rates_dirty = false;
    _damping_coefficient = damping_coeff;
    _spring_rate = spring_rate;
}

float LNVehicleSuspensionSettings::get_spring_rate(const float p_sprung_mass) const {
    if (suspension_rates_dirty) {
        const_cast<LNVehicleSuspensionSettings*>(this)->_recalculate_suspension_rates(p_sprung_mass);
    }
    return _spring_rate;
}

float LNVehicleSuspensionSettings::get_damping_coefficient(const float p_sprung_mass) const {
    if (suspension_rates_dirty) {
        const_cast<LNVehicleSuspensionSettings*>(this)->_recalculate_suspension_rates(p_sprung_mass);
    }

    return _damping_coefficient;
}

void LNVehicleSuspensionSettings::_bind_methods() {
    MAKE_BIND_FLOAT(LNVehicleSuspensionSettings, maximum);
    MAKE_BIND_FLOAT(LNVehicleSuspensionSettings, minimum);
    MAKE_BIND_FLOAT(LNVehicleSuspensionSettings, natural_frequency_hz);
    MAKE_BIND_FLOAT(LNVehicleSuspensionSettings, rest);
    MAKE_BIND_FLOAT(LNVehicleSuspensionSettings, damping_ratio);
}
