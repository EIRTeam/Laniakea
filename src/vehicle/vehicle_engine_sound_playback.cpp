#include "vehicle_engine_sound_playback.h"

#include "godot_cpp/classes/audio_server.hpp"
#include "godot_cpp/classes/audio_stream_playback_resampled.hpp"
#include "godot_cpp/classes/audio_stream_wav.hpp"
#include "godot_cpp/core/math.hpp"
#include "godot_cpp/variant/packed_vector2_array.hpp"

void LNVehicleEngineSoundPlayback::stop_audio_sample(std::optional<SoundPlaybackSample> &r_sample) {
	DEV_ASSERT(r_sample.has_value());
	r_sample->state = SampleState::FADING_OUT;

	outgoing_samples.push_back(*r_sample);

	r_sample.reset();
}

void LNVehicleEngineSoundPlayback::stop_audio_map(std::optional<SoundPlaybackMapAtThrottle> &r_map) {
	if (!r_map.has_value()) {
		return;
	}

	if (r_map->higher.has_value()) {
		stop_audio_sample(r_map->higher);
	}
	if (r_map->lower.has_value()) {
		stop_audio_sample(r_map->lower);
	}
}

void LNVehicleEngineSoundPlayback::update_audio_map(std::optional<SoundPlaybackMapAtThrottle> &r_map, int p_desired_map_idx) {
	playback_update_mutex->lock();
	if (p_desired_map_idx == -1) {
		stop_audio_map(r_map);
		r_map.reset();
		playback_update_mutex->unlock();
		return;
	}

	if (r_map.has_value() && r_map->map_idx != p_desired_map_idx) {
		stop_audio_map(r_map);
		r_map.reset();
	}

	if (!r_map.has_value()) {
		r_map = SoundPlaybackMapAtThrottle {
			.map_idx = p_desired_map_idx
		};
	}

	auto [lower_idx, higher_idx] = sound_config->find_sample_range(r_map->map_idx, rpm);

	update_audio_sample(r_map->lower, p_desired_map_idx, lower_idx);
	update_audio_sample(r_map->higher, p_desired_map_idx, higher_idx);

	r_map->volume_linear = 1.0f;
	playback_update_mutex->unlock();
}

void LNVehicleEngineSoundPlayback::update_audio_map_params(SoundPlaybackMapAtThrottle &r_map) {
	if (r_map.lower.has_value() && r_map.higher.has_value()) {
		const float crossfade_percentage = compute_crossfade_t(rpm, r_map.lower->rpm_min, r_map.lower->rpm_max, r_map.higher->rpm_min, r_map.higher->rpm_max, 1.0f);
		float vol_a = Math::cos(crossfade_percentage * (Math_PI / 2.0f)); // 1.0 → 0.0
		float vol_b = Math::sin(crossfade_percentage * (Math_PI / 2.0f)); // 0.0 → 1.0
		r_map.lower->volume = vol_a * r_map.volume_linear;
		r_map.higher->volume = vol_b * r_map.volume_linear;
	} else if (r_map.lower.has_value()) {
		r_map.lower->volume = r_map.volume_linear;
	} else if (r_map.higher.has_value()) {
		r_map.higher->volume = r_map.volume_linear;
	}

	if (r_map.lower.has_value()) {
		const float pitch = rpm / r_map.lower->rpm_max;
		r_map.lower->pitch_scale = pitch;
	}

	if (r_map.higher.has_value()) {
		const float pitch = rpm / r_map.higher->rpm_max;
		r_map.higher->pitch_scale = pitch;
	}
}

void LNVehicleEngineSoundPlayback::update_audio_sample(std::optional<SoundPlaybackSample> &r_sample, int p_map_idx, int p_sample_idx) {
	if (p_sample_idx == -1) {
		if (r_sample.has_value()) {
			stop_audio_sample(r_sample);
		}
		return;
	}

	if (r_sample.has_value() && r_sample->sample_idx == p_sample_idx) {
		return; // Already playing the correct sample
	}

	if (r_sample.has_value()) {
		stop_audio_sample(r_sample);
	}

	Ref<AudioStream> stream = sound_config->get_map_sample_stream(p_map_idx, p_sample_idx);
	const float pitch = rpm / static_cast<float>(sound_config->get_map_sample_rpm(p_map_idx, p_sample_idx));

	Ref<AudioStreamPlaybackResampled> playback = stream->instantiate_playback();
	playback->start();
	playback->begin_resample();

	Ref<AudioStreamWAV> wav = stream;

	DEV_ASSERT(wav.is_valid());

	if (wav.is_valid()) {
		DEV_ASSERT(wav->get_loop_mode() == AudioStreamWAV::LOOP_FORWARD);
	}

	r_sample = SoundPlaybackSample {
		.playback = playback,
		.sample_idx = p_sample_idx,
		.rpm_min = p_sample_idx == 0 ? 0 : sound_config->get_map_sample_rpm(p_map_idx, p_sample_idx - 1),
		.rpm_max = sound_config->get_map_sample_rpm(p_map_idx, p_sample_idx),
		.state = SampleState::FADING_IN,
	};
}

Ref<LNEngineSoundConfiguration> LNVehicleEngineSoundPlayback::get_sound_config() const {
	return sound_config;
}

void LNVehicleEngineSoundPlayback::set_sound_config(const Ref<LNEngineSoundConfiguration> &sound_config_) {
	sound_config = sound_config_;
}

float LNVehicleEngineSoundPlayback::compute_crossfade_t(float p_value,
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

void LNVehicleEngineSoundPlayback::update(float p_throttle, float p_rpm, float p_delta) {
	effective_throttle = p_throttle;
	rpm = p_rpm;

	Vector<int> throttles;
	for (int i = 0; i < sound_config->get_map_count(); i++) {
		throttles.push_back(sound_config->get_map_throttle_percentage(i));
	}

	int needed_map_higher = MIN(throttles.bsearch(static_cast<int>(effective_throttle * 100.0f), true), throttles.size() - 1);
	int needed_map_lower = needed_map_higher - 1;

	update_audio_map(current_sound_playback.lower_throttle_map, needed_map_lower);
	update_audio_map(current_sound_playback.higher_throttle_map, needed_map_higher);

	// Throttle crossfade
	if (current_sound_playback.lower_throttle_map.has_value() && current_sound_playback.higher_throttle_map.has_value()) {
		int throttle_lower = sound_config->get_map_throttle_percentage(current_sound_playback.lower_throttle_map->map_idx);
		int throttle_higher = sound_config->get_map_throttle_percentage(current_sound_playback.higher_throttle_map->map_idx);
		int throttle_lower_min = current_sound_playback.lower_throttle_map->map_idx == 0 ? 0 : sound_config->get_map_throttle_percentage(current_sound_playback.lower_throttle_map->map_idx - 1);
		const float crossfade = compute_crossfade_t(effective_throttle * 100.0f, throttle_lower_min, throttle_lower, throttle_lower, throttle_higher, 0.25f);
		float vol_a = Math::cos(crossfade * (Math_PI / 2.0f)); // 1.0 → 0.0
		float vol_b = Math::sin(crossfade * (Math_PI / 2.0f)); // 0.0 → 1.0
		current_sound_playback.lower_throttle_map->volume_linear = vol_a;
		current_sound_playback.higher_throttle_map->volume_linear = vol_b;
	}

	if (current_sound_playback.lower_throttle_map.has_value()) {
		update_audio_map_params(*current_sound_playback.lower_throttle_map);
	}
	if (current_sound_playback.higher_throttle_map.has_value()) {
		update_audio_map_params(*current_sound_playback.higher_throttle_map);
	}
}

int32_t LNVehicleEngineSoundPlayback::write_map_frames(SoundPlaybackMapAtThrottle &p_map, AudioFrame *r_out, int32_t frame_count) {
	int32_t max_written_frames = 0;
	if (p_map.lower.has_value()) {
		int32_t written_frames = write_sample_frames(*p_map.lower, frame_count, r_out);
		max_written_frames = MAX(max_written_frames, written_frames);
		p_map.lower->state = SampleState::PLAYING;
	}
	if (p_map.higher.has_value()) {
		int32_t written_frames = write_sample_frames(*p_map.higher, frame_count, r_out);
		max_written_frames = MAX(max_written_frames, written_frames);
		p_map.higher->state = SampleState::PLAYING;
	}
	return max_written_frames;
}

int32_t LNVehicleEngineSoundPlayback::write_sample_frames(const SoundPlaybackSample &p_playback_sample, int32_t p_frame_count, AudioFrame *r_out) {
	DEV_ASSERT(p_playback_sample.playback.is_valid());
	PackedVector2Array frames = p_playback_sample.playback->mix_audio(p_playback_sample.pitch_scale, p_frame_count);

	const float mix_rate = AudioServer::get_singleton()->get_mix_rate();
	constexpr float FADE_MS = 10.0f;
	const float fade_frame_count = MIN(Math::ceil((mix_rate / 1000.0f) * FADE_MS), frames.size() - 1);

	for (int i = 0; i < frames.size(); i++) {
		float volume = p_playback_sample.volume;

		if (p_playback_sample.state == FADING_IN || p_playback_sample.state == FADING_OUT) {
			if (frames.size() <= 1) {
				volume = 0.0f;
			} else {
				// fade in/out
				const float from = p_playback_sample.state == FADING_IN ? 0.0f : fade_frame_count;
				const float to = p_playback_sample.state == FADING_IN ? fade_frame_count : 0.0f;
				const float vol_fade = CLAMP(Math::inverse_lerp(from, to, i), 0.0, 1.0f);
				volume *= vol_fade;
			}
		}
		r_out[i].left += frames[i].x * volume;
		r_out[i].right += frames[i].y * volume;
	}

	return frames.size();
}

void LNVehicleEngineSoundPlayback::_stop() {
	playing = false;
}

void LNVehicleEngineSoundPlayback::_start(double p_from_pos) {
	playing = true;
	playback_update_mutex.instantiate();
}

bool LNVehicleEngineSoundPlayback::_is_playing() const {
	return playing;
}

double LNVehicleEngineSoundPlayback::_get_playback_position() const {
	return 0.0f;
}

int32_t LNVehicleEngineSoundPlayback::_mix(AudioFrame *p_buffer, float p_rate_scale, int32_t p_frames) {
	playback_update_mutex->lock();

	for (int32_t i = 0; i < p_frames; i++) {
		p_buffer[i].left = 0.0f;
		p_buffer[i].right = 0.0f;
	}

	int32_t written_frames = 0;

	for (int i = outgoing_samples.size() - 1; i >= 0; i--) {
		written_frames = MAX(written_frames, write_sample_frames(outgoing_samples[i], p_frames, p_buffer));
		outgoing_samples.remove_at_unordered(i);
		DEV_ASSERT(written_frames == p_frames);
	}

	if (!current_sound_playback.lower_throttle_map.has_value() && !current_sound_playback.higher_throttle_map.has_value()) {
		// nothing to play, write empty
		for (int i = 0; i < p_frames; i++) {
			p_buffer[i].left = 0.0f;
			p_buffer[i].right = 0.0f;
		}
		playback_update_mutex->unlock();
		return p_frames;
	}
	{
		if (current_sound_playback.lower_throttle_map.has_value()) {
			written_frames = MAX(written_frames, write_map_frames(*current_sound_playback.lower_throttle_map, p_buffer, p_frames));
		}

		if (current_sound_playback.higher_throttle_map.has_value()) {
			written_frames = MAX(written_frames, write_map_frames(*current_sound_playback.higher_throttle_map, p_buffer, p_frames));
		}
	}
	DEV_ASSERT(written_frames == p_frames);

	playback_update_mutex->unlock();
	return written_frames;
}
