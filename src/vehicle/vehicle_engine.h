#pragma once

#include "bind_macros.h"
#include "godot_cpp/classes/audio_stream_playback_polyphonic.hpp"
#include "godot_cpp/classes/audio_stream_player.hpp"
#include "godot_cpp/classes/audio_stream_polyphonic.hpp"
#include "vehicle/vehicle_engine_settings.h"
#include "vehicle/vehicle_engine_sound.h"
class LNVehicleEngine : public RefCounted {
    GDCLASS(LNVehicleEngine, RefCounted);
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
                          float lower_min, float lower_max,
                          float upper_min, float upper_max,
                          float n);

public:
    MAKE_SETTER_GETTER_VALUE(Ref<LNVehicleEngineSettings>, engine_settings, engine_settings);

    void update(float p_throttle, float p_clutch_torque, double p_delta);
    void set_rpm(float p_rpm);
    float get_rpm() const;
    static void _bind_methods();

    void set_audio_stream_player(AudioStreamPlayer *p_audio_stream_player);
};