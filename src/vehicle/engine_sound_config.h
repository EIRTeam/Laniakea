#pragma once

#include "godot_cpp/classes/audio_stream.hpp"
#include "godot_cpp/classes/resource.hpp"
#include "godot_cpp/variant/dictionary.hpp"

using namespace godot;

class LNEngineSoundConfiguration : public Resource {
	GDCLASS(LNEngineSoundConfiguration, Resource);
	using RPM = int;

	struct EngineAudioSample {
		int rpm;
		Ref<AudioStream> stream;
	};
	struct EngineAudioMap {
		int throttle;
		bool order_dirty = false;
		Vector<EngineAudioSample> samples;
	};

	struct SampleComparator {
		_FORCE_INLINE_ bool operator()(const EngineAudioSample &p_a, const EngineAudioSample &p_b) const {
			return p_a.rpm < p_b.rpm;
		}
	};

	struct AudioMapComparator {
		_FORCE_INLINE_ bool operator()(const EngineAudioMap &p_a, const EngineAudioMap &p_b) const {
			return p_a.throttle < p_b.throttle;
		}
	};

	bool maps_order_dirty = false;
	LocalVector<EngineAudioMap> engine_audio_maps;
	void _ensure_sorted();

public:
	Dictionary get_samples_bind() const;
	void set_samples_bind(const Dictionary &p_data);
	void add_sound(int p_throttle, int p_rpm, Ref<AudioStream> p_sound_stream);

	int get_map_count() const;
	int get_map_throttle_percentage(int p_map) const;
	int get_map_sample_count(int p_map) const;
	int get_map_sample_rpm(int p_map, int p_sample) const;
	Ref<AudioStream> get_map_sample_stream(int p_map, int p_sample) const;
	Pair<int, int> find_sample_range(int p_map, int p_rpm) const;

	static void _bind_methods();
};
