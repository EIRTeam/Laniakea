#include "vehicle_differential.h"

#include "vehicle/debug_icons.h"
#include "vehicle/vehicle_drivetrain_config.h"
int LNVehicleDifferential::get_output_count() const {
	return 2;
}
LNVehicleShaft::UpstreamData LNVehicleDifferential::get_upstream_data() {
	UpstreamData downstream_data_left = get_child(0)->get_upstream_data();
	UpstreamData downstream_data_right = get_child(1)->get_upstream_data();

	const float upstream_velocity = (downstream_data_left.angular_velocity + downstream_data_right.angular_velocity) * 0.5f;

	const float downstream_inertia = downstream_data_left.inertia + downstream_data_right.inertia;
	const float ratio = drivetrain_settings->get_final_ratio();

	downstream_datas[0] = downstream_data_left;
	downstream_datas[1] = downstream_data_right;

	return {
		.inertia = drivetrain_settings->get_differential_inertia() + (downstream_inertia / (ratio * ratio)),
		.angular_velocity = upstream_velocity * ratio,
	};
}

void LNVehicleDifferential::apply_downstream(const DownstreamData &p_data) {
	/*const float ratio = drivetrain_settings->get_final_ratio();
	const float total_input_torque = p_data.torque * ratio;

	// The inertia coming from the gearbox, transformed to the axle side
	float inertia_from_upstream = p_data.reflected_inertia * (ratio * ratio);

	// 1. Total System sum
	float total_inertia = downstream_datas[0].inertia + downstream_datas[1].inertia + inertia_from_upstream;
	float total_net_torque = total_input_torque + downstream_datas[0].net_reaction_torque + downstream_datas[1].net_reaction_torque;

	// 2. Singular acceleration
	float shared_acceleration = total_net_torque / total_inertia;

	// 3. Torque distribution (Conserves Momentum)
	float torque_to_left  = (downstream_datas[0].inertia * shared_acceleration) - downstream_datas[0].net_reaction_torque;
	float torque_to_right = (downstream_datas[1].inertia * shared_acceleration) - downstream_datas[1].net_reaction_torque;

	// 4. Correct Reflected Inertia for children
	// Each child sees the drivetrain AND the other wheel as its "upstream" mass.
	float upstream_for_left  = inertia_from_upstream + downstream_datas[1].inertia;
	float upstream_for_right = inertia_from_upstream + downstream_datas[0].inertia;

	get_child(0)->apply_downstream({
		.torque = torque_to_left,
		.reflected_inertia = upstream_for_left
	});

	get_child(1)->apply_downstream({
		.torque = torque_to_right,
		.reflected_inertia = upstream_for_right
	});*/

	const float ratio = drivetrain_settings->get_final_ratio();
	const float differential_inertia = drivetrain_settings->get_differential_inertia();

	float inertia = 0.0f;

	if (ratio != 0.0f) {
		float total_downstream_inertia = (p_data.reflected_inertia + differential_inertia) * (ratio * ratio);

		inertia = total_downstream_inertia * 0.5f;
	}

	float total_torque = (p_data.torque * drivetrain_settings->get_final_ratio());
	float left = 0.5f * total_torque + downstream_datas[0].net_reaction_torque;
	float right = 0.5f * total_torque + downstream_datas[1].net_reaction_torque;

	switch (drivetrain_settings->get_differential_type()) {
		case LNVehicleDrivetrainSettings::DifferentialType::OPEN: {
			// Nothing, split is already correct
		} break;
		case LNVehicleDrivetrainSettings::DifferentialType::LOCKED: {
			const float estimated_angular_velocity_x = downstream_datas[0].angular_velocity + (left / downstream_datas[0].inertia) * delta;
			const float estimated_angular_velocity_y = downstream_datas[1].angular_velocity + (right / downstream_datas[1].inertia) * delta;
			// Then how much torque do we need to add to make the wheels perfectly locked
			float angular_delta = (estimated_angular_velocity_x - estimated_angular_velocity_y) * 0.5f;
			float diff_locking_torque_x = downstream_datas[0].inertia * (angular_delta) / delta;
			float diff_locking_torque_y = downstream_datas[1].inertia * angular_delta / delta;
			left -= diff_locking_torque_x;
			right += diff_locking_torque_y;
		}
	}

	left -= downstream_datas[0].net_reaction_torque;
	right -= downstream_datas[1].net_reaction_torque;

	get_child(0)->apply_downstream({
			.torque = left,
			.reflected_inertia = inertia,
	});

	get_child(1)->apply_downstream({
			.torque = right,
			.reflected_inertia = inertia,
	});
}

bool LNVehicleDifferential::has_input() const {
	return true;
}

String LNVehicleDifferential::get_debugger_display_name() const {
	return String::utf8(LNDebugIcons::VECTOR_DIFFERENCE) + " " + get_name();
}

void LNVehicleDifferential::pre_update(float p_delta, const VehicleInputState &p_input_state) {
	delta = p_delta;
}

String LNVehicleDifferential::get_debug_text() const {
	return "Type: LOCKED";
}
