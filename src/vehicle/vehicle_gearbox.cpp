#include "vehicle_gearbox.h"
#include "vehicle/debug_icons.h"
#include "vehicle/shaft.h"

bool LNVehicleGearbox::has_input() const {
    return true;
}

int LNVehicleGearbox::get_output_count() const {
    return 1;
}

int LNVehicleGearbox::get_current_gear() const {
    return current_gear;
}

LNVehicleShaft::UpstreamData LNVehicleGearbox::get_upstream_data() {
    const float ratio = drivetrain_settings->get_gear_ratio(current_gear);

    LNVehicleShaft::UpstreamData downstream_data = get_child(0)->get_upstream_data();

    if (ratio != 0.0f) {
        return {
            .inertia = (downstream_data.inertia / (ratio * ratio)) + drivetrain_settings->get_gearbox_inertia(),
            .angular_velocity = downstream_data.angular_velocity * ratio,
            .net_reaction_torque = downstream_data.net_reaction_torque / ratio
        };
    }
    return {};
}

void LNVehicleGearbox::apply_downstream(const DownstreamData &p_data) {
    const float ratio = drivetrain_settings->get_gear_ratio(current_gear);

    float net_upstream_inertia = p_data.reflected_inertia + drivetrain_settings->get_gearbox_inertia();
    get_child(0)->apply_downstream({
        .torque = p_data.torque * ratio,
        .reflected_inertia = net_upstream_inertia * (ratio * ratio)
    });
}

void LNVehicleGearbox::pre_update(float p_delta, const VehicleInputState &p_input_state) {
    if (p_input_state.gear != current_gear) {
        current_gear = p_input_state.gear;
    }
}

String LNVehicleGearbox::get_debugger_display_name() const {
    return String::utf8(LNDebugIcons::GEARBOX) + " " + get_name();
}

String LNVehicleGearbox::get_debug_text() const {
    String gear_str = vformat("%d", current_gear);
    if (current_gear == 0) {
        gear_str = "N";
    } else if (current_gear == -1) {
        gear_str = "R";
    }
    return vformat("Gear: %s", gear_str);
}
