#pragma once

#include "godot_cpp/classes/audio_frame.hpp"
#include "godot_cpp/classes/audio_stream_playback.hpp"
#include "godot_cpp/classes/audio_stream_playback_resampled.hpp"
#include "godot_cpp/classes/mutex.hpp"
#include "vehicle/engine_sound_config.h"

#include <optional>

class LNVehicleEngineSoundPlayback : public AudioStreamPlayback {
	GDCLASS(LNVehicleEngineSoundPlayback, AudioStreamPlayback)
	Ref<LNEngineSoundConfiguration> sound_config;

	enum SampleState {
		FADING_IN,
		PLAYING,
		FADING_OUT
	};

	struct SoundPlaybackSample {
		Ref<AudioStreamPlaybackResampled> playback;
		float volume = 0.0f;
		float pitch_scale = 1.0f;
		int sample_idx;
		int rpm_min = 0;
		int rpm_max = 0;
		SampleState state;
	};

	struct SoundPlaybackMapAtThrottle {
		int map_idx;
		float volume_linear = 0.0f;
		std::optional<SoundPlaybackSample> lower;
		std::optional<SoundPlaybackSample> higher;
	};

	Ref<Mutex> playback_update_mutex;
	struct SoundPlaybackInformation {
		std::optional<SoundPlaybackMapAtThrottle> lower_throttle_map;
		std::optional<SoundPlaybackMapAtThrottle> higher_throttle_map;
	} current_sound_playback;

	LocalVector<SoundPlaybackSample> sample_playbacks;

	float effective_throttle = 0.0f;
	float rpm = 0.0f;

	void stop_audio_sample(std::optional<SoundPlaybackSample> &r_sample);
	void stop_audio_map(std::optional<SoundPlaybackMapAtThrottle> &r_map);
	int32_t write_map_frames(SoundPlaybackMapAtThrottle &p_map, AudioFrame *r_out, int32_t frame_count);
	int32_t write_sample_frames(const SoundPlaybackSample &p_playback_sample, int32_t p_frame_count, AudioFrame *r_out);
	void update_audio_map(std::optional<SoundPlaybackMapAtThrottle> &r_map, int p_desired_map_idx);
	void update_audio_map_params(SoundPlaybackMapAtThrottle &r_map);
	void update_audio_sample(std::optional<SoundPlaybackSample> &r_sample, int p_map_idx, int p_sample_idx);

	LocalVector<SoundPlaybackSample> outgoing_samples;

	float compute_crossfade_t(float p_value,
			float lower_min,
			float lower_max,
			float upper_min,
			float upper_max,
			float n);

	bool playing = false;

public:
	void update(float p_throttle, float p_rpm, float p_delta);
	virtual void _start(double p_from_pos) override;
	virtual void _stop() override;
	virtual bool _is_playing() const override;
	virtual double _get_playback_position() const override;
	virtual int32_t _mix(AudioFrame *p_buffer, float p_rate_scale, int32_t p_frames) override;

	Ref<LNEngineSoundConfiguration> get_sound_config() const;
	void set_sound_config(const Ref<LNEngineSoundConfiguration> &p_sound_config);

	static void _bind_methods() {}
};
