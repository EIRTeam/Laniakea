#include "vehicle_drivetrain.h"

void LNVehicleDrivetrain::update_clutch(float p_clutch_pedal_input, float p_velocity_in_rads, float p_velocity_from_gearbox, float p_delta) {
    bool in_gear = get_current_gear() != 0;
    float clutch_engagement = 1.0f - p_clutch_pedal_input;
    float velocity_engine = p_velocity_in_rads;
    float velocity_transmission = p_velocity_from_gearbox;

    //Calculate slip
    float slip = 0.0f;
    if (in_gear)
    {
        slip = velocity_engine - velocity_transmission;
    }
    else
    {
        slip = 0.0f;
    }

    //Calculate torque
    const float clutch_stiffness = 15.0f;
    const float clutch_damping = 8.0f;
    float torque = clutch_engagement * slip * clutch_stiffness; //tau = omega * k
    current_clutch_torque += (torque - current_clutch_torque) * clutch_damping * p_delta;
    current_clutch_torque = CLAMP(current_clutch_torque, -400.0f, 400.0f); //Make sure it doesn't exceed the torque capacity of the clutch
    UtilityFunctions::prints(current_clutch_torque);
}

float LNVehicleDrivetrain::clutch_get_upstream_torque() const {
    if (current_gear == 0) return 0.0f;
    return current_clutch_torque;
}

float LNVehicleDrivetrain::gearbox_angular_vel_to_upstream(float p_angular_vel) const {
    return p_angular_vel * drivetrain_settings->get_gear_ratio(current_gear);
}

float LNVehicleDrivetrain::gearbox_get_downstream_torque(float p_engine_torque) const {
    if (current_gear == 0) return 0.0f;

    const float gear_ratio = drivetrain_settings->get_gear_ratio(current_gear);
    const float input_torque = current_clutch_torque;
    return input_torque * gear_ratio;
}

Pair<float, float> LNVehicleDrivetrain::differential_get_downstream_torque(float p_gearbox_torque) const {
    const float half_torque = p_gearbox_torque * drivetrain_settings->get_final_ratio() * 0.5f;
    return {half_torque, half_torque};
}

float LNVehicleDrivetrain::differential_get_upstream_angular_velocity(float p_wheel_angular_velocity_left, float p_wheel_angular_velocity_right) const {
    return (p_wheel_angular_velocity_left + p_wheel_angular_velocity_right) * 0.5f * drivetrain_settings->get_final_ratio();
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