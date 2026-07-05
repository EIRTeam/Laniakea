#include "vehicle_engine_sound_stream.h"

#include "vehicle/vehicle_engine_sound.h"

Ref<AudioStreamPlayback> LNVehicleEngineSoundStream::_instantiate_playback() const {
	Ref<LNVehicleEngineSoundPlayback> playback;
	playback.instantiate();
	playback->set_sound_config(sound_config);

	return playback;
}
