#include "vehicle_engine.h"
#include "bind_macros.h"
#include "math.h"
void LNVehicleEngine::update(float p_throttle, float p_clutch_torque, double p_delta) {
    time += p_delta;
    
    const float current_rpm = LNMath::AV_2_RPM * angular_velocity;

    if (current_rpm >= engine_settings->get_rpm_limit()) {
        next_limiter_bounce = time + (1.0f / static_cast<float>(engine_settings->get_power_cut_frequency_hz()));
    }

    const bool in_limiter_bounce = next_limiter_bounce > time;

    const float coast_torque = engine_settings->sample_coast_curve(current_rpm);

    float target_torque_ratio = coast_torque / engine_settings->sample_torque_curve(current_rpm);
    
    const float remapped_throttle = in_limiter_bounce ? 0.0 : engine_settings->sample_throttle(current_rpm, p_throttle);
    float final_throttle = remapped_throttle;
    
    const float idle_rpm = engine_settings->get_idle_rpm();
    // Idle magic
    if (current_rpm < idle_rpm) {
        float idle_throttle = engine_settings->sample_throttle(current_rpm, engine_settings->inverse_sample_torque(target_torque_ratio, current_rpm));


        static constexpr float active_idle_range = 0.25f;

        float idle_fade_rpm = CLAMP(Math::inverse_lerp(
            idle_rpm + (engine_settings->get_rpm_limit() - idle_rpm) * active_idle_range,
            idle_rpm,
            current_rpm
        ), 0.0f, 1.0f);
        final_throttle = Math::lerp(idle_throttle * idle_fade_rpm, 1.0f, remapped_throttle);
    }

    const float current_torque = engine_settings->sample_torque_curve(current_rpm, final_throttle) - p_clutch_torque;
    angular_velocity += (current_torque / engine_settings->get_inertia()) * p_delta;
    angular_velocity = MAX(angular_velocity, 0.0f);
    output_torque = current_torque;
}

void LNVehicleEngine::set_rpm(float p_rpm) {
    angular_velocity = p_rpm * LNMath::RPM_2_AV;
}

float LNVehicleEngine::get_rpm() const {
    return angular_velocity * LNMath::AV_2_RPM;
}

void LNVehicleEngine::_bind_methods() {
    ClassDB::bind_method(D_METHOD("update", "throttle", "clutch_torque", "delta"), &LNVehicleEngine::update);
    ClassDB::bind_method(D_METHOD("set_rpm", "rpm"), &LNVehicleEngine::set_rpm);
    ClassDB::bind_method(D_METHOD("get_rpm"), &LNVehicleEngine::get_rpm);
    MAKE_BIND_RESOURCE(LNVehicleEngine, engine_settings, "LNVehicleEngineSettings");
}
