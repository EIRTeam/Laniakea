#include "vehicle_engine_sound.h"

#include "godot_cpp/core/math.hpp"

void VehicleEngineSound::stop_audio_map(std::optional<SoundPlaybackMap> &r_map) {
	if (!r_map.has_value()) {
		return;
	}

	if (r_map->higher.has_value()) {
		stream_playback->set_stream_volume(r_map->higher->voice_idx, Math::linear2db(0.0f));
		stream_playback->stop_stream(r_map->higher->voice_idx);
	}
	if (r_map->lower.has_value()) {
		stream_playback->set_stream_volume(r_map->lower->voice_idx, Math::linear2db(0.0f));
		stream_playback->stop_stream(r_map->lower->voice_idx);
	}
}

void VehicleEngineSound::update_audio_map(std::optional<SoundPlaybackMap> &r_map, int p_desired_map_idx) {
	if (p_desired_map_idx == -1) {
		stop_audio_map(r_map);
		r_map.reset();
		return;
	}

	if (r_map.has_value() && r_map->map_idx != p_desired_map_idx) {
		stop_audio_map(r_map);
		r_map.reset();
	}

	if (!r_map.has_value()) {
		r_map = SoundPlaybackMap {
			.map_idx = p_desired_map_idx
		};
	}

	auto [lower_idx, higher_idx] = sound_config->find_sample_range(r_map->map_idx, rpm);

	update_audio_sample(r_map->lower, p_desired_map_idx, lower_idx);
	update_audio_sample(r_map->higher, p_desired_map_idx, higher_idx);

	r_map->volume_linear = 1.0f;
}

void VehicleEngineSound::update_audio_map_params(SoundPlaybackMap &r_map) {
	if (r_map.lower.has_value() && r_map.higher.has_value()) {
		const float crossfade_percentage = compute_crossfade_t(rpm, r_map.lower->rpm_min, r_map.lower->rpm_max, r_map.higher->rpm_min, r_map.higher->rpm_max, 1.0f);
		float vol_a = Math::cos(crossfade_percentage * (Math_PI / 2.0f)); // 1.0 → 0.0
		float vol_b = Math::sin(crossfade_percentage * (Math_PI / 2.0f)); // 0.0 → 1.0
		stream_playback->set_stream_volume(r_map.lower->voice_idx, Math::linear2db(vol_a * r_map.volume_linear));
		stream_playback->set_stream_volume(r_map.higher->voice_idx, Math::linear2db(vol_b * r_map.volume_linear));
	} else if (r_map.lower.has_value()) {
		stream_playback->set_stream_volume(r_map.lower->voice_idx, Math::linear2db(r_map.volume_linear));
	} else if (r_map.higher.has_value()) {
		stream_playback->set_stream_volume(r_map.higher->voice_idx, Math::linear2db(r_map.volume_linear));
	}

	if (r_map.lower.has_value()) {
		const float pitch = rpm / r_map.lower->rpm_max;
		stream_playback->set_stream_pitch_scale(r_map.lower->voice_idx, pitch);
	}

	if (r_map.higher.has_value()) {
		const float pitch = rpm / r_map.higher->rpm_max;
		stream_playback->set_stream_pitch_scale(r_map.higher->voice_idx, pitch);
	}
}

void VehicleEngineSound::update_audio_sample(std::optional<SoundPlaybackSample> &r_sample, int p_map_idx, int p_sample_idx) {
	if (p_sample_idx == -1) {
		if (r_sample.has_value()) {
			stream_playback->stop_stream(r_sample->voice_idx);
			r_sample.reset();
		}
		return;
	}

	if (r_sample.has_value() && r_sample->sample_idx == p_sample_idx) {
		return; // Already playing the correct sample
	}

	if (r_sample.has_value()) {
		stream_playback->stop_stream(r_sample->voice_idx);
	}

	Ref<AudioStream> stream = sound_config->get_map_sample_stream(p_map_idx, p_sample_idx);
	const float pitch = rpm / static_cast<float>(sound_config->get_map_sample_rpm(p_map_idx, p_sample_idx));

	r_sample = SoundPlaybackSample {
		.voice_idx = stream_playback->play_stream(stream, Math::linear2db(0.0f)),
		.sample_idx = p_sample_idx,
		.rpm_min = p_sample_idx == 0 ? 0 : sound_config->get_map_sample_rpm(p_map_idx, p_sample_idx - 1),
		.rpm_max = sound_config->get_map_sample_rpm(p_map_idx, p_sample_idx),
	};
}

Ref<LNEngineSoundConfiguration> VehicleEngineSound::get_sound_config() const {
	return sound_config;
}

void VehicleEngineSound::set_sound_config(const Ref<LNEngineSoundConfiguration> &sound_config_) {
	sound_config = sound_config_;
}

float VehicleEngineSound::compute_crossfade_t(float p_value,
		float p_lower_min, float p_lower_max,
		float p_upper_min, float p_upper_max,
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

void VehicleEngineSound::update(float p_throttle, float p_rpm, float p_delta) {
	effective_throttle = p_throttle;
	rpm = p_rpm;

	Vector<int> throttles;
	for (int i = 0; i < sound_config->get_map_count(); i++) {
		throttles.push_back(sound_config->get_map_throttle_percentage(i));
	}

	int needed_map_higher = MIN(throttles.bsearch(static_cast<int>(effective_throttle * 100.0f), true), throttles.size() - 1);
	int needed_map_lower = needed_map_higher - 1;

	update_audio_map(sound_playback.lower_map, needed_map_lower);
	update_audio_map(sound_playback.higher_map, needed_map_higher);

	// Throttle crossfade
	if (sound_playback.lower_map.has_value() && sound_playback.higher_map.has_value()) {
		int throttle_lower = sound_config->get_map_throttle_percentage(sound_playback.lower_map->map_idx);
		int throttle_higher = sound_config->get_map_throttle_percentage(sound_playback.higher_map->map_idx);
		int throttle_lower_min = sound_playback.lower_map->map_idx == 0 ? 0 : sound_config->get_map_throttle_percentage(sound_playback.lower_map->map_idx - 1);
		const float crossfade = compute_crossfade_t(effective_throttle * 100.0f, throttle_lower_min, throttle_lower, throttle_lower, throttle_higher, 0.25f);
		float vol_a = Math::cos(crossfade * (Math_PI / 2.0f)); // 1.0 → 0.0
		float vol_b = Math::sin(crossfade * (Math_PI / 2.0f)); // 0.0 → 1.0
		sound_playback.lower_map->volume_linear = vol_a;
		sound_playback.higher_map->volume_linear = vol_b;
	}

	if (sound_playback.lower_map.has_value()) {
		update_audio_map_params(*sound_playback.lower_map);
	}
	if (sound_playback.higher_map.has_value()) {
		update_audio_map_params(*sound_playback.higher_map);
	}
}

Ref<AudioStreamPlaybackPolyphonic> VehicleEngineSound::get_stream_playback() const {
	return stream_playback;
}

void VehicleEngineSound::set_stream_playback(const Ref<AudioStreamPlaybackPolyphonic> &stream_playback_) {
	stream_playback = stream_playback_;
}
