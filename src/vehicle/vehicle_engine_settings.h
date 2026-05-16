#pragma once

#include "bind_macros.h"
#include "godot_cpp/classes/resource.hpp"
#include "vehicle/engine_sound_config.h"
#include <optional>

using namespace godot;

class LNVehicleEngineSettings : public Resource {
    GDCLASS(LNVehicleEngineSettings, Resource);
    float coast_ref_rpm = 0.0f;
    float coast_ref_torque = 0.0f;
    float coast_ref_nonlinearity = 0.0f;
    float rpm_limit = 7500.0f;
    int power_cut_frequency_hz = 20;
    float inertia = 0.12f;
    float idle_rpm = 1000.0f;
    Ref<LNEngineSoundConfiguration> sound_config;

    struct BakedCurveData {
        float peak_power = 0.0f;
        LocalVector<Pair<float, float>> torque_curve;
    };

    std::optional<BakedCurveData> baked_curve_data;
    
    String torque_curve_text;

    void _ensure_torque_curve_baked();
    void set_torque_curve_bind(String p_torque_curve);
    String get_torque_curve_bind() const;
public:

    MAKE_SETTER_GETTER_FLOAT_VALUE(coast_ref_rpm, coast_ref_rpm);
    MAKE_SETTER_GETTER_FLOAT_VALUE(coast_ref_torque, coast_ref_torque);
    MAKE_SETTER_GETTER_FLOAT_VALUE(coast_ref_nonlinearity, coast_ref_nonlinearity);
    MAKE_SETTER_GETTER_FLOAT_VALUE(rpm_limit, rpm_limit);
    MAKE_SETTER_GETTER_FLOAT_VALUE(idle_rpm, idle_rpm);
    MAKE_SETTER_GETTER_FLOAT_VALUE(inertia, inertia);
    MAKE_SETTER_GETTER_VALUE(int, power_cut_frequency_hz, power_cut_frequency_hz);
    MAKE_SETTER_GETTER_VALUE(Ref<LNEngineSoundConfiguration>, sound_config, sound_config);

    static void _bind_methods();
    float sample_torque_curve(float p_rpm) const;
    float sample_coast_curve(float p_rpm) const;
    float sample_throttle(float p_rpm, float p_throttle) const;
    float sample_torque_curve(float p_rpm, float p_throttle) const;
    float inverse_sample_torque(float p_torque_ratio, float p_rpm) const;
};