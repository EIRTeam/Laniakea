#pragma once

#include "bind_macros.h"
#include "vehicle/shaft.h"
#include "vehicle/vehicle_drivetrain_config.h"
class LNVehicleDifferential : public LNVehicleShaft {
	GDCLASS(LNVehicleDifferential, LNVehicleShaft);

	UpstreamData downstream_datas[2];
	float delta = 0.0f;
	Ref<LNVehicleDrivetrainSettings> drivetrain_settings;

public:
	MAKE_SETTER_GETTER_VALUE(Ref<LNVehicleDrivetrainSettings>, drivetrain_settings, drivetrain_settings);
	virtual int get_output_count() const override;
	virtual UpstreamData get_upstream_data() override;
	virtual void apply_downstream(const DownstreamData &p_data) override;
	virtual bool has_input() const override;
	virtual String get_debugger_display_name() const override;
	virtual void pre_update(float p_delta, const VehicleInputState &p_input_state) override;
	virtual String get_debug_text() const override;
	static void _bind_methods() {}
};
