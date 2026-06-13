#pragma once

#include "vehicle/shaft.h"
#include "vehicle/vehicle_drivetrain_config.h"

class LNVehicleClutchNode : public LNVehicleShaft {
	GDCLASS(LNVehicleClutchNode, LNVehicleShaft);

public:
	float current_torque = 0.0f;
	virtual void update(float p_delta, const VehicleInputState &p_input_state) override;
	virtual int get_output_count() const override;
	virtual bool has_input() const override;
	virtual String get_debugger_display_name() const override;
	static void _bind_methods() {}
	virtual String get_debug_text() const override;
};
