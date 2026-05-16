#pragma once

#include "godot_cpp/classes/audio_stream_playback_polyphonic.hpp"
#include "vehicle/engine_sound_config.h"
#include <optional>

class VehicleEngineSound {
    Ref<LNEngineSoundConfiguration> sound_config;
    Ref<AudioStreamPlaybackPolyphonic> stream_playback;

    struct SoundPlaybackSample {
        int64_t voice_idx;
        float volume = 0.0f;
        int sample_idx;
        int rpm_min = 0;
        int rpm_max = 0;
    };

    struct SoundPlaybackMap {
        int map_idx;
        float volume_linear = 0.0f;
        std::optional<SoundPlaybackSample> lower;
        std::optional<SoundPlaybackSample> higher;
    };

    struct SoundPlaybackInformation {
        std::optional<SoundPlaybackMap> lower_map;
        std::optional<SoundPlaybackMap> higher_map;
    } sound_playback;
    
    float effective_throttle = 0.0f;
    float rpm = 0.0f;

    void stop_audio_map(std::optional<SoundPlaybackMap> &r_map);
    void update_audio_map(std::optional<SoundPlaybackMap> &r_map, int p_desired_map_idx);
    void update_audio_map_params(SoundPlaybackMap &r_map);
    void update_audio_sample(std::optional<SoundPlaybackSample> &r_sample, int p_map_idx, int p_sample_idx);

    float compute_crossfade_t(float p_value,
                          float lower_min, float lower_max,
                          float upper_min, float upper_max,
                          float n);

public:
    void update(float p_throttle, float p_rpm, float p_delta);

    Ref<LNEngineSoundConfiguration> get_sound_config() const;
    void set_sound_config(const Ref<LNEngineSoundConfiguration> &p_sound_config);

    Ref<AudioStreamPlaybackPolyphonic> get_stream_playback() const;
    void set_stream_playback(const Ref<AudioStreamPlaybackPolyphonic> &stream_playback_);
};