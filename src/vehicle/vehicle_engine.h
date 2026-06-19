#pragma once

#include "bind_macros.h"
#include "godot_cpp/classes/audio_stream_playback_polyphonic.hpp"
#include "godot_cpp/classes/audio_stream_player.hpp"
#include "godot_cpp/classes/audio_stream_polyphonic.hpp"
#include "vehicle/shaft.h"
#include "vehicle/vehicle_engine_settings.h"
#include "vehicle/vehicle_engine_sound.h"
class LNVehicleEngine : public LNVehicleShaft {
	GDCLASS(LNVehicleEngine, LNVehicleShaft);
	Ref<LNVehicleEngineSettings> engine_settings;

	float output_torque = 0.0f;
	float angular_velocity = 0.0f;
	float time = 0.0f;
	float next_limiter_bounce = 0.0f;
	float effective_throttle = 0.0f;

	Ref<AudioStreamPolyphonic> audio_stream;
	AudioStreamPlayer *asp = nullptr;

	VehicleEngineSound sound;

	float compute_crossfade_t(float p_value,
			float lower_min,
			float lower_max,
			float upper_min,
			float upper_max,
			float n);

	float clutch_reaction_torque = 0.0f;

public:
	MAKE_SETTER_GETTER_VALUE(Ref<LNVehicleEngineSettings>, engine_settings, engine_settings);
	virtual void initialize(VehicleTelemetry *p_telemetry) override;
	void update_output_torque(float p_throttle, double p_delta);
	void integrate_angular_velocity(float p_clutch_reaction_torque, float p_extra_inertia, double p_delta);
	void set_rpm(float p_rpm);
	float get_rpm() const;
	float get_angular_velocity() const;
	float get_output_torque() const;
	static void _bind_methods();
	virtual String get_debugger_display_name() const override;

	void set_audio_stream_player(AudioStreamPlayer *p_audio_stream_player);

	virtual UpstreamData get_upstream_data() override;
	virtual int get_output_count() const override;
	virtual bool has_input() const override;
	virtual void pre_update(float p_delta, const VehicleInputState &p_input_state) override;
	virtual void update(float p_delta, const VehicleInputState &p_input_state) override;
	virtual void apply_reaction(const DownstreamData &p_data) override;
	virtual String get_debug_text() const override;
};
