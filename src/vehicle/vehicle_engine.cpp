#include "vehicle_engine.h"

#include "bind_macros.h"
#include "debug_icons.h"
#include "godot_cpp/core/math.hpp"
#include "math.h"
#include "vehicle/shaft.h"

#include <algorithm>
#include <functional>

float LNVehicleEngine::compute_crossfade_t(float p_value,
		float p_lower_min,
		float p_lower_max,
		float p_upper_min,
		float p_upper_max,
		float p_n) {
	float xfade_start = p_lower_max - p_n * (p_lower_max - p_lower_min);
	float xfade_end = p_upper_min + p_n * (p_upper_max - p_upper_min);

	if (p_value <= xfade_start) {
		return 0.0f;
	}
	if (p_value >= xfade_end) {
		return 1.0f;
	}

	return (p_value - xfade_start) / (xfade_end - xfade_start);
}

void LNVehicleEngine::update_output_torque(float p_throttle, double p_delta) {
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
	if (current_rpm < idle_rpm && current_rpm > 0.0f) {
		float idle_throttle = engine_settings->sample_throttle(current_rpm, engine_settings->inverse_sample_torque(target_torque_ratio, current_rpm));

		static constexpr float active_idle_range = 0.25f;

		float idle_fade_rpm = CLAMP(Math::inverse_lerp(
											idle_rpm + (engine_settings->get_rpm_limit() - idle_rpm) * active_idle_range,
											idle_rpm,
											current_rpm),
				0.0f,
				1.0f);
		final_throttle = Math::lerp(idle_throttle * idle_fade_rpm, 1.0f, remapped_throttle);
	}

	effective_throttle = final_throttle;

	const float gross_torque = engine_settings->sample_torque_curve(current_rpm, final_throttle);
	output_torque = gross_torque;
}

void LNVehicleEngine::integrate_angular_velocity(float p_clutch_reaction_torque, float p_extra_inertia, double p_delta) {
	// const float current_torque = output_torque - p_clutch_reaction_torque;
	const float current_torque = output_torque + p_clutch_reaction_torque;
	angular_velocity += (current_torque / (engine_settings->get_inertia() + p_extra_inertia)) * p_delta;
	angular_velocity = MAX(angular_velocity, 0.0f);

	if (asp != nullptr) {
		sound.set_sound_config(engine_settings->get_sound_config());
		sound.set_stream_playback(asp->get_stream_playback());
		sound.update(effective_throttle, get_rpm(), p_delta);
	}
}

void LNVehicleEngine::set_rpm(float p_rpm) {
	angular_velocity = p_rpm * LNMath::RPM_2_AV;
}

void LNVehicleEngine::set_audio_stream_player(AudioStreamPlayer *p_audio_stream_player) {
	asp = p_audio_stream_player;
	if (asp != nullptr) {
		audio_stream.instantiate();
		asp->set_stream(audio_stream);
		asp->play();
	}
}

LNVehicleShaft::UpstreamData LNVehicleEngine::get_upstream_data() {
	return {
		.inertia = engine_settings->get_inertia(),
		.angular_velocity = get_angular_velocity()
	};
}

int LNVehicleEngine::get_output_count() const {
	return 0;
}

bool LNVehicleEngine::has_input() const {
	return true;
}

void LNVehicleEngine::pre_update(float p_delta, const VehicleInputState &p_input_state) {
	update_output_torque(p_input_state.throttle, p_delta);
}

void LNVehicleEngine::update(float p_delta, const VehicleInputState &p_input_state) {
	integrate_angular_velocity(clutch_reaction_torque, 0.0f, p_delta);
}

void LNVehicleEngine::apply_reaction(const DownstreamData &p_data) {
	clutch_reaction_torque = p_data.torque;
	// TODO: Clutch inertia
}

String LNVehicleEngine::get_debug_text() const {
	return vformat("RPM: %d\nEffective throttle: %.2f%%\nOutput torque: %.2f N m", get_rpm(), effective_throttle * 100.0f, output_torque);
}

float LNVehicleEngine::get_rpm() const {
	return angular_velocity * LNMath::AV_2_RPM;
}

float LNVehicleEngine::get_angular_velocity() const {
	return angular_velocity;
}

float LNVehicleEngine::get_output_torque() const {
	return output_torque;
}

void LNVehicleEngine::_bind_methods() {
	ClassDB::bind_method(D_METHOD("update", "throttle", "clutch_torque", "extra_inertia", "delta"), &LNVehicleEngine::update_output_torque);
	ClassDB::bind_method(D_METHOD("set_rpm", "rpm"), &LNVehicleEngine::set_rpm);
	ClassDB::bind_method(D_METHOD("set_audio_stream_player", "asp"), &LNVehicleEngine::set_audio_stream_player);
	ClassDB::bind_method(D_METHOD("get_rpm"), &LNVehicleEngine::get_rpm);
	MAKE_BIND_RESOURCE(LNVehicleEngine, engine_settings, "LNVehicleEngineSettings");
}

String LNVehicleEngine::get_debugger_display_name() const {
	return String::utf8(LNDebugIcons::ENGINE) + " " + get_name();
}
