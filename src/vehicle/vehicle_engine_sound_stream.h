#pragma once

#include "bind_macros.h"
#include "godot_cpp/classes/audio_stream.hpp"
#include "godot_cpp/classes/audio_stream_playback.hpp"
#include "vehicle/engine_sound_config.h"

using namespace godot;

class LNVehicleEngineSoundStream : public AudioStream {
	GDCLASS(LNVehicleEngineSoundStream, AudioStream);

public:
	Ref<LNEngineSoundConfiguration> sound_config;
	MAKE_SETTER_GETTER_VALUE(Ref<LNEngineSoundConfiguration>, sound_config, sound_config);
	static void _bind_methods() {}
	virtual Ref<AudioStreamPlayback> _instantiate_playback() const override;
};
