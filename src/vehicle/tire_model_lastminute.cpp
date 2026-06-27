#include "tire_model_lastminute.h"

#include "godot_cpp/core/defs.hpp"
#include "godot_cpp/core/math.hpp"

using namespace godot;

float LNVehicleTyreLastMinute::contact_half_length(float p_vertical_load) const {
	const float dz = CLAMP(p_vertical_load / kz, 0.0f, 2.0f * get_radius());
	return Math::sqrt(dz * (2.0f * get_radius() - dz));
}

float LNVehicleTyreLastMinute::ux(float p_sx) const {
	const float vsx = p_sx * vf;
	const float vsx_vs = (vsx / vs);
	return ux_min + (ux_max - ux_min) / (1.0f + vsx_vs * vsx_vs);
}

float LNVehicleTyreLastMinute::uy(float p_sy) const {
	const float vsy = p_sy * vf;
	const float vsy_vs = (vsy / vs);
	return uy_min + (uy_max - uy_min) / (1.0f + vsy_vs * vsy_vs);
}

float LNVehicleTyreLastMinute::x_c(float p_sx, float p_sy, float p_vertical_load) const {
	const float a = contact_half_length(p_vertical_load);
	float three_a = a * a * a;

	const float x = (kx * p_sx * uy_max) * (kx * p_sx * uy_max);
	const float y = (ky * p_sy * ux_max) * (ky * p_sy * ux_max);

	const float num = 4.0f * three_a * Math::sqrt(x + y);

	const float den = 3.0f * p_vertical_load * ux_max * uy_max;

	return num / den - a;
}
LNVehicleTyreLastMinute::ForcesResult LNVehicleTyreLastMinute::forces(float p_sx, float p_sy, float p_vertical_load) const {
	const float a = contact_half_length(p_vertical_load);
	const float xc = x_c(p_sx, p_sy, p_vertical_load);

	const float a_sq = a * a;
	const float a_plus_xc_sq = (a + xc) * (a + xc);
	const float a_minus_xc_sq = (a - xc) * (a - xc);
	const float a_cubed = a_sq * a;
	const float xc_sq = xc * xc;
	const float sx_sq = p_sx * p_sx;
	const float sy_sq = p_sy * p_sy;

	const float smag = Math::sqrt(sx_sq + sy_sq);

	const float smag_safe = Math::is_zero_approx(smag) ? 1.0f : smag;

	const float ux_val = ux(p_sx);
	const float uy_val = uy(p_sy);

	const bool sticking = xc < a;

	// --- Fx ---
	const float Fx_stick =
			0.5f * kx * p_sx * a_minus_xc_sq + (ux_val * p_sx / smag_safe) * ((p_vertical_load * (2.0f * a - xc) * a_plus_xc_sq) / (4.0f * a_cubed));

	const float Fx_slide = (ux_val * p_sx / smag_safe) * p_vertical_load;
	float fx = sticking ? Fx_stick : Fx_slide;
	fx = Math::is_zero_approx(smag) ? 0.0f : fx;

	// --- Fy ---
	const float Fy_stick =
			0.5f * ky * p_sy * a_minus_xc_sq + (uy_val * p_sy / smag_safe) * ((p_vertical_load * (2.0f * a - xc) * a_plus_xc_sq) / (4.0f * a_cubed));
	const float Fy_slide = (uy_val * p_sy / smag_safe) * p_vertical_load;
	float fy = sticking ? Fy_stick : Fy_slide;
	fy = Math::is_zero_approx(smag) ? 0.0f : fy;

	// --- Mz ---
	// NOTE on source fidelity: the Desmos "post integration" Mz slide
	// branch is 0, and its stick branch's slide-region contribution uses
	// u_x(s_y) (a transcription quirk in the original — u_x evaluated at
	// the *lateral* slip). We reproduce that faithfully here rather than
	// silently fixing it, since this is a translation of the given model.
	const float ux_val_at_sy = ux(p_sy);
	const float Mz_stick =
			(1.0f / 6.0f) * ky * p_sy * a_minus_xc_sq * (a + 2.0f * xc) + (ux_val_at_sy * p_sy / smag_safe) * (-3.0f * fz0 * ((a_sq - xc_sq) * (a_sq - xc_sq))) / (16.0f * a_cubed);
	const float Mz_slide = 0.0f;
	float mz = sticking ? Mz_stick : Mz_slide;
	mz = Math::is_zero_approx(smag) ? 0.0 : mz;

	return {
		.lateral = fy,
		.longitudinal = fx,
		.self_centering_torque = mz
	};
}

LNVehicleTyreLastMinute::LNVehicleTyreLastMinute() {
	const float dz0 = fz0 / kz;
	const float a0 = Math::sqrt(dz0 * (2.0f * get_radius() - dz0));
	kx = c_x / (2.0f * Math::pow(a0, 2.0f));
	ky = c_y / (2.0f * Math::pow(a0, 2.0f));
}

void LNVehicleTyreLastMinute::_bind_methods() {
}
