#include "vehicle_engine_settings.h"
#include "bind_macros.h"
#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "math.h"

void LNVehicleEngineSettings::_ensure_torque_curve_baked() {
    PackedStringArray arr = torque_curve_text.split("\n");
    baked_curve_data = BakedCurveData {};
    baked_curve_data->torque_curve.resize(arr.size()+1);
    baked_curve_data->torque_curve[0] = {0.0f, 0.0f};
    for (int i = 1; i < arr.size(); i++) {
        PackedStringArray torque_entry = arr[i].split("|");
        
        baked_curve_data->torque_curve[i] = {0.0f, 0.0f};

        if (!Engine::get_singleton()->is_editor_hint()) {
            ERR_CONTINUE_MSG(torque_entry.size() != 2, "Invalid torque entry");
            ERR_CONTINUE_MSG(!torque_entry[0].is_valid_float(), "Invalid torque entry");
            ERR_CONTINUE_MSG(!torque_entry[1].is_valid_float(), "Invalid torque entry");
        } else {    
            if (torque_entry.size() != 2) continue;
            if (torque_entry[0].is_valid_float()) continue;
            if (torque_entry[1].is_valid_float()) continue;
        }

        const float rpm = static_cast<float>(torque_entry[0].to_float());
        const float torque = static_cast<float>(torque_entry[1].to_float());

        baked_curve_data->torque_curve[i] = { rpm, torque };
        baked_curve_data->peak_power = MAX(baked_curve_data->peak_power, torque * rpm * LNMath::RPM_2_AV);
    }
}

void LNVehicleEngineSettings::set_torque_curve_bind(String p_torque_curve) {
    torque_curve_text = p_torque_curve;
    baked_curve_data.reset();
}

String LNVehicleEngineSettings::get_torque_curve_bind() const {
    return torque_curve_text;
}

void LNVehicleEngineSettings::_bind_methods() {
    MAKE_BIND_FLOAT(LNVehicleEngineSettings, coast_ref_rpm);
    MAKE_BIND_FLOAT(LNVehicleEngineSettings, coast_ref_torque);
    MAKE_BIND_FLOAT(LNVehicleEngineSettings, coast_ref_nonlinearity);
    MAKE_BIND_FLOAT(LNVehicleEngineSettings, rpm_limit);
    MAKE_BIND_INT(LNVehicleEngineSettings, power_cut_frequency_hz);
    MAKE_BIND_RESOURCE(LNVehicleEngineSettings, sound_config, LNEngineSoundConfiguration);

	ClassDB ::bind_method(D_METHOD("set_"
								   "torque_curve",
								   "torque_curve"),
						  &LNVehicleEngineSettings ::set_torque_curve_bind);
	ClassDB ::bind_method(D_METHOD("get_"
								   "torque_curve"),
						  &LNVehicleEngineSettings ::get_torque_curve_bind);
	;
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "torque_curve", PROPERTY_HINT_MULTILINE_TEXT), "set_torque_curve", "get_torque_curve");
}

float LNVehicleEngineSettings::sample_torque_curve(float p_rpm) const {
    const_cast<LNVehicleEngineSettings*>(this)->_ensure_torque_curve_baked();
    if (baked_curve_data->torque_curve.is_empty()) {
        return 0.f;
    }
    for (int i = 1; i < baked_curve_data->torque_curve.size(); i++) {
        float prev_rpm = baked_curve_data->torque_curve[i-1].first;
        float curr_rpm = baked_curve_data->torque_curve[i].first;
        if (prev_rpm <= p_rpm && p_rpm <= curr_rpm) {
            const float prev_torque = baked_curve_data->torque_curve[i-1].second;
            const float curr_torque = baked_curve_data->torque_curve[i].second;
            const float x = Math::inverse_lerp(prev_rpm, curr_rpm, p_rpm);
            return Math::lerp(prev_torque, curr_torque, x);
        }
    }

    return baked_curve_data->torque_curve[baked_curve_data->torque_curve.size()-1].second;
}

float LNVehicleEngineSettings::sample_coast_curve(float p_rpm) const {
    return Math::pow(p_rpm / coast_ref_rpm, 1.0f + coast_ref_nonlinearity) * coast_ref_torque;
}

float LNVehicleEngineSettings::sample_throttle(float p_rpm, float p_throttle) const {
    if (p_throttle == 1.0f) {
        return 1.0f;
    }

    const float peak_torque = sample_torque_curve(p_rpm);
    const float angular_vel = p_rpm * LNMath::AV_2_RPM;
    const float max_coast_torque = sample_coast_curve(p_rpm);
	const float remapped_throttle = LNMath::smoothmin(
        MAX(p_throttle + p_throttle * baked_curve_data->peak_power / (peak_torque * angular_vel + 1e-30) * (1.0f - p_throttle),0.0f),
        1.0f,
        (1.0f - p_throttle)
    );
    return remapped_throttle;
}

float LNVehicleEngineSettings::sample_torque_curve(float p_rpm, float p_throttle) const {
    const float max_coast_torque = sample_coast_curve(p_rpm);
    const float peak_torque = sample_torque_curve(p_rpm);

    return Math::lerp(-max_coast_torque, peak_torque, CLAMP(p_throttle, 0.0f, 1.0f));
}

float LNVehicleEngineSettings::inverse_sample_torque(float p_torque_ratio, float p_rpm) const {
    if (p_torque_ratio >= 1.0f) return 1.0f;
    if (p_torque_ratio <= 0.0f) return 0.0f;

    const float peak_torque = sample_torque_curve(p_rpm);
    const float angular_vel = p_rpm * LNMath::AV_2_RPM;
    const float K  = baked_curve_data->peak_power / (peak_torque * angular_vel + 1e-30f);
    const float K2 = K * K;

    // Seed: invert the inner quadratic q = t*(1 + K*(1-t)), ignoring the Kt/2 shift.
    // Kt² - (1+K)t + r = 0  →  take the smaller root.
    float t;
    if (K < 1e-6f) {
        t = p_torque_ratio;
    } else {
        const float disc = (1.0f + K) * (1.0f + K) - 4.0f * K * p_torque_ratio;
        t = ((1.0f + K) - Math::sqrt(MAX(disc, 0.0f))) / (2.0f * K);
    }
    t = CLAMP(t, 0.0f, 1.0f);

    // Newton on the exact cubic — no numerical derivative needed.
    // r(t)  = (K²/4)t³ - (K + K²/4)t²  + (1+K)t
    // r'(t) = (3K²/4)t² - (2K + K²/2)t + (1+K)
    for (int i = 0; i < 4; ++i) {
        const float r  = t * (K2/4.0f * t*t - (K + K2/4.0f) * t + (1.0f + K));
        const float dr = K2*0.75f * t*t - (2.0f*K + K2*0.5f) * t + (1.0f + K);

        const float err = r - p_torque_ratio;
        if (Math::abs(err) < 1e-7f) break;
        if (Math::abs(dr)  < 1e-10f) break;

        t = CLAMP(t - err / dr, 0.0f, 1.0f);
    }
    return t;
}
