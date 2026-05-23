#include "vehicle_drivetrain.h"
#include "godot_cpp/core/error_macros.hpp"

static void ApplyTbrClamp(const float total, const float tbr, float &r_left, float &r_right)
{
    float absTotal = Math::abs(total);
    if (absTotal < 1e-5f) { r_left = r_right = 0.0f; return; }

    float aL = Math::abs(r_left);
    float aR = Math::abs(r_right);
    float hi = MAX(aL, aR);
    float lo = MIN(aL, aR);

    if (lo < 1e-6f || hi > lo * tbr)
    {
        float sign = SIGN(total);
        float low  = absTotal / (1.0f + tbr); // hi = tbr*low, hi+low=total
        float high = absTotal - low;

        bool leftWasHigh = (aL >= aR);
        r_left  = sign * (leftWasHigh ? high : low);
        r_right = sign * (leftWasHigh ? low  : high);
    }
}

float LNVehicleDrivetrain::gearbox_get_downstream_angular_velocity() const {
    if (current_gear == 0) return 0.0f;

    const float gear_ratio = drivetrain_settings->get_gear_ratio(current_gear);
    return gearbox_input_shaft_angular_vel / gear_ratio;
}

float LNVehicleDrivetrain::gearbox_get_downstream_torque() const {
    if (current_gear == 0) return 0.0f;

    const float gear_ratio = drivetrain_settings->get_gear_ratio(current_gear);
    const float input_torque = current_clutch_torque;
    return input_torque * gear_ratio;
}

Pair<float, float> LNVehicleDrivetrain::differential_get_downstream_torque(float p_gearbox_torque) const {
    const float half_torque = p_gearbox_torque * drivetrain_settings->get_final_ratio() * 0.5f;
    return {half_torque, half_torque};
}

void LNVehicleDrivetrain::gear_up() {
    current_gear = MIN(current_gear + 1, drivetrain_settings->get_positive_gearbox_ratios().size());
}

void LNVehicleDrivetrain::gear_down() {
    current_gear = MAX(current_gear - 1, -drivetrain_settings->get_negative_gearbox_ratios().size());
}

int LNVehicleDrivetrain::get_current_gear() const {
    return current_gear;
}

void LNVehicleDrivetrain::update(Ref<LNVehicleEngine> p_engine, float p_throttle, float p_clutch_pedal_input, float p_wheel_angular_velocity_left, float p_wheel_angular_velocity_right, float p_wheel_inertia, float p_delta) {
    DEV_ASSERT(p_engine.is_valid());
    
    p_engine->update_output_torque(p_throttle, p_delta);

    float inertia_reflected_on_gearbox_output = 0.0f;

    diff_get_upstream_angular_velocity_and_inertia(differential_angular_velocity, inertia_reflected_on_gearbox_output,
        p_wheel_angular_velocity_left, p_wheel_angular_velocity_right,
        p_wheel_inertia, p_wheel_inertia);
    float reflected_inertia_on_clutch = 0.0f;
    gearbox_get_upstream_angular_velocity_and_inertia(gearbox_input_shaft_angular_vel, reflected_inertia_on_clutch, differential_angular_velocity, inertia_reflected_on_gearbox_output);

    const float clutch_inertia = 0.0f;
    
    const float engine_inertia = p_engine->get_engine_settings()->get_inertia();
    const float engine_side_inertia = (0.5f * clutch_inertia + engine_inertia);
    const float load_side_inertia = (0.5f * clutch_inertia) + reflected_inertia_on_clutch;
    const float engine_momentum = p_engine->get_angular_velocity() * engine_side_inertia;
    const float load_momentum = gearbox_input_shaft_angular_vel * load_side_inertia;
    const float total_momentum = engine_momentum + load_momentum;

    float full_lock_angular_velocity = total_momentum / (engine_side_inertia + load_side_inertia);

    float full_lock_torque = engine_side_inertia * (full_lock_angular_velocity - p_engine->get_angular_velocity()) / p_delta;
    
    float limit = drivetrain_settings->get_clutch_max_torque() * (1.0f - p_clutch_pedal_input);
    current_clutch_torque = -CLAMP(full_lock_torque, -limit, limit);

    p_engine->integrate_angular_velocity(-current_clutch_torque, 0.0f, p_delta);
}

void LNVehicleDrivetrain::gearbox_get_upstream_angular_velocity_and_inertia(float &r_upstream_angular_vel, float &r_upstream_inertia, float p_velocity_downstream, float p_inertia_downstream) {
    const float ratio = drivetrain_settings->get_gear_ratio(get_current_gear());

    if (ratio != 0.0f) {
        r_upstream_angular_vel = p_velocity_downstream * ratio;
        r_upstream_inertia = drivetrain_settings->get_gearbox_inertia() + (p_inertia_downstream / (ratio * ratio));
    } else {
        r_upstream_angular_vel = 0.0f;
        r_upstream_inertia = 0.0f;
    }
}

void LNVehicleDrivetrain::diff_get_upstream_angular_velocity_and_inertia(float &r_upstream_angular_vel, float &r_upstream_inertia, float p_velocity_downstream_left, float p_velocity_downstream_right, float p_inertia_downstream_left, float p_inertia_downstream_right) {
    float output_vel = (p_velocity_downstream_left + p_velocity_downstream_right) * 0.5f;

    float downstrema_inertia = p_inertia_downstream_left + p_inertia_downstream_right;

    const float ratio = drivetrain_settings->get_final_ratio();

    r_upstream_angular_vel = output_vel * ratio;
    const float differential_inertia = 0.0f;
    r_upstream_inertia = differential_inertia + (downstrema_inertia / (ratio * ratio));
}

void LNVehicleDrivetrain::gearbox_get_downstream_inertia(
        float &r_downstream_inertia, 
        float p_inertia_upstream) 
{
    const float ratio = drivetrain_settings->get_gear_ratio(get_current_gear());

    if (ratio != 0.0f) {
        float net_upstream_inertia = p_inertia_upstream + drivetrain_settings->get_gearbox_inertia();
        r_downstream_inertia = MAX(0.0f, net_upstream_inertia * (ratio * ratio));
    } else {
        r_downstream_inertia = 0.0f;
    }
}

void LNVehicleDrivetrain::diff_get_downstream_inertia(
    float &r_down_inertia_left, float &r_down_inertia_right,
    float p_inertia_upstream) 
{
    const float ratio = drivetrain_settings->get_final_ratio();
    const float differential_inertia = 0.0f;

    if (ratio != 0.0f) {
        float total_downstream_inertia = (p_inertia_upstream + differential_inertia) * (ratio * ratio);
        
        r_down_inertia_left = total_downstream_inertia * 0.5f;
        r_down_inertia_right = total_downstream_inertia * 0.5f;
    } else {
        r_down_inertia_left = r_down_inertia_right = 0.0f;
    }
}

Vector2 LNVehicleDrivetrain::GetDownstreamTorque(float p_gearbox_torque, Vector2 p_wheel_angular_velocities, Vector2 p_extra_torques, Vector2 p_wheel_inertias, float p_delta)
{
    float total_torque = (p_gearbox_torque * drivetrain_settings->get_final_ratio());
    float left  = 0.5f * total_torque + p_extra_torques.x;
    float right = 0.5f * total_torque + p_extra_torques.y;


    Vector2 omega = p_wheel_angular_velocities;
    LNDifferentialSettings settings_test;

    // WIP: Allow changing diffs
    switch (DifferentialType::LOCKED)
    {
        case DifferentialType::LSD:
        {
            /*float dOmega = omega.x - omega.y; 
            float lockCoef = (total >= 0f) ? diffData.lsdLockCoefAccel : diffData.lsdLockCoefDecel;

            float coupling = diffData.lsdPreloadNm + Mathf.Abs(dOmega) * lockCoef;
            coupling = Mathf.Min(coupling, diffData.lsdMaxCouplingNm);

            if (dOmega > 0f) { left -= coupling; right += coupling; }    
            else if (dOmega < 0f) { left += coupling; right -= coupling; }

            ApplyTbrClamp(total, diffData.lsdTorqueBiasRatio, ref left, ref right);*/
            break;
        }


        case DifferentialType::LOCKED:
        {
            // First we calculate the angular velocity if the wheels continued as-is
            const float estimated_angular_velocity_x = p_wheel_angular_velocities.x + (left / p_wheel_inertias.x) * p_delta; 
            const float estimated_angular_velocity_y = p_wheel_angular_velocities.y + (right / p_wheel_inertias.y) * p_delta; 
            // Then how much torque do we need to add to make the wheels perfectly locked
            float angular_delta = (estimated_angular_velocity_x - estimated_angular_velocity_y) * 0.5f;
            float diff_locking_torque_x = p_wheel_inertias.x * (angular_delta) / p_delta;
            float diff_locking_torque_y = p_wheel_inertias.y * angular_delta / p_delta;
            left -= diff_locking_torque_x;
            right += diff_locking_torque_y;


            DEV_ASSERT(Math::is_finite(left));
            DEV_ASSERT(Math::is_finite(right));
            /*float dOmega = omega.x - omega.y;
            float coupling = MIN(Math::abs(dOmega) * settings_test.lockStiffness, settings_test.lockMaxCouplingNm);

            if (dOmega > 0.0f) { left -= coupling; right += coupling; }
            else if (dOmega < 0.0f) { left += coupling; right -= coupling; }

            ApplyTbrClamp(total_torque, 10.0f, left, right);
            */
            break;
        }
    }

    return Vector2(left, right);
}
