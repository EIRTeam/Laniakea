#include "clutch.h"

#include "math.h"
#include "vehicle/debug_icons.h"

void LNVehicleClutchNode::update(float p_delta, const VehicleInputState &p_input_state) {
	LNVehicleShaft *drive_side = get_child(0);
	LNVehicleShaft *load_side = get_child(1);

	LNVehicleShaft::UpstreamData drive_side_data = drive_side->get_upstream_data();

	LNVehicleShaft::UpstreamData load_side_data = load_side->get_upstream_data();

	const float autoclutch_min = drivetrain_settings->get_autoclutch_min();
	const float autoclutch_max = drivetrain_settings->get_autoclutch_max();
	const float autoclutch_amount = 1.0f - CLAMP(Math::inverse_lerp(autoclutch_min, autoclutch_max, drive_side_data.angular_velocity * LNMath::AV_2_RPM), 0.0f, 1.0f);

	const float clutch_input = MAX(p_input_state.clutch, autoclutch_amount);

	const float clutch_inertia = 0.0f;

	float drive_side_inertia = (0.5f * clutch_inertia + drive_side_data.inertia);
	float load_side_inertia = (0.5f * clutch_inertia + load_side_data.inertia);

	const float drive_momentum = drive_side_data.angular_velocity * drive_side_data.inertia;
	const float load_momentum = load_side_data.angular_velocity * load_side_data.inertia;
	const float total_momentum = drive_momentum + load_momentum;

	float full_lock_angular_velocity = total_momentum / (drive_side_inertia + load_side_inertia);
	float full_lock_torque = drive_side_inertia * (full_lock_angular_velocity - drive_side_data.angular_velocity) / p_delta;

	float limit = drivetrain_settings->get_clutch_max_torque() * (1.0f - clutch_input);
	current_torque = -CLAMP(full_lock_torque, -limit, limit);

	drive_side->apply_reaction({
			.torque = -current_torque,
			.reflected_inertia = clutch_inertia,
	});
	load_side->apply_downstream({
			.torque = current_torque,
			.reflected_inertia = clutch_inertia,
	});
}

int LNVehicleClutchNode::get_output_count() const {
	return 2;
}

bool LNVehicleClutchNode::has_input() const {
	return false;
}

String LNVehicleClutchNode::get_debugger_display_name() const {
	return String::utf8(LNDebugIcons::CLUTCH) + " " + get_name();
}

String LNVehicleClutchNode::get_debug_text() const {
	return vformat("Torque: %.2f N m", current_torque);
}
