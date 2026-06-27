#pragma once

#include "vehicle/vehicle_tyre.h"

// Borrowed from https://www.desmos.com/calculator/zzxo710fqd

class LNVehicleTyreLastMinute : public LNVehicleTyre {
	GDCLASS(LNVehicleTyreLastMinute, LNVehicleTyre);
	float fz0 = 2500.0f;
	float c_x = 50000.0f;
	float c_y = 40000.0f;
	float kz = 200000.0f;
	float ux_min = 0.9f;
	float ux_max = 1.3f;
	float uy_min = 1.0f;
	float uy_max = 1.2f;
	float vs = 7.2f;
	float vf = 100.0f / 3.6f;

	// Calculated
	float kx = 0.0f;
	float ky = 0.0f;

	float contact_half_length(float p_vertical_load) const;
	float ux(float p_sx) const;
	float uy(float p_sy) const;
	float x_c(float p_sx, float p_sy, float p_vertical_load) const;

public:
	virtual ForcesResult forces(float p_sx, float p_sy, float p_vertical_load) const override;

	LNVehicleTyreLastMinute();
	static void _bind_methods();
};
