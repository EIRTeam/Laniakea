#include "engine_sound_config.h"
#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/core/defs.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/core/object.hpp"
#include "godot_cpp/core/property_info.hpp"
#include "godot_cpp/variant/dictionary.hpp"
#include "godot_cpp/variant/typed_array.hpp"

#include <algorithm>

Dictionary LNEngineSoundConfiguration::get_samples_bind() const {
    Dictionary out;
    TypedArray<Dictionary> samples_out;
    for (int i = 0; i < engine_audio_maps.size(); i++) {
        int throttle = engine_audio_maps[i].throttle;
        for (int j = 0; j < engine_audio_maps[i].samples.size(); j++) {
            Dictionary dict;
            dict["throttle"] = throttle;
            dict["rpm"] = engine_audio_maps[i].samples[j].rpm;
            dict["stream"] = engine_audio_maps[i].samples[j].stream;
            samples_out.push_back(dict);
        }
    }

    out["samples"] = samples_out;
    return out;
}

void LNEngineSoundConfiguration::set_samples_bind(const Dictionary &p_data) {
    TypedArray<Dictionary> engine_samples = p_data.get("samples", TypedArray<Dictionary>());

    for (int i = 0; i < engine_samples.size(); i++) {
        Dictionary sample = engine_samples[i];
        ERR_CONTINUE_MSG(!sample.has_all(Array { "rpm", "stream", "throttle" }), "Sample didn't have all required properties");
        int throttle = sample["throttle"];
        int rpm = sample["rpm"];
        Ref<AudioStream> stream = sample["stream"];

        add_sound(throttle, rpm, stream);
    }

    _ensure_sorted();
}

void LNEngineSoundConfiguration::add_sound(int p_throttle, int p_rpm, Ref<AudioStream> p_sound_stream) {
    EngineAudioMap *audio_map = nullptr;
    {
        auto it = std::find_if(engine_audio_maps.begin(), engine_audio_maps.end(), [p_throttle](const EngineAudioMap &p_map) {
            return p_map.throttle == p_throttle;
        });

        if (it == engine_audio_maps.end()) {
            engine_audio_maps.push_back(EngineAudioMap {
                .throttle = p_throttle
            });
            audio_map = &engine_audio_maps[engine_audio_maps.size()-1];
            maps_order_dirty = true;
        } else {
            audio_map = &*it;
        }
    }

    audio_map->samples.push_back(EngineAudioSample {
        .rpm = p_rpm,
        .stream = p_sound_stream
    });
    audio_map->order_dirty = true;
}

void LNEngineSoundConfiguration::_ensure_sorted() {
    if (maps_order_dirty) {
        engine_audio_maps.sort_custom<AudioMapComparator>();
        maps_order_dirty = false;
    }

    for (EngineAudioMap &map : engine_audio_maps) {
        if (map.order_dirty) {
            map.samples.sort_custom<SampleComparator>();
        }
    }
}

int LNEngineSoundConfiguration::get_map_count() const {
    return engine_audio_maps.size();
}

int LNEngineSoundConfiguration::get_map_throttle_percentage(int p_map) const {
    ERR_FAIL_INDEX_V(p_map, engine_audio_maps.size(), -1);
    return engine_audio_maps[p_map].throttle;
}

int LNEngineSoundConfiguration::get_map_sample_count(int p_map) const {
    return engine_audio_maps[p_map].samples.size();
}

int LNEngineSoundConfiguration::get_map_sample_rpm(int p_map, int p_sample) const {
    ERR_FAIL_INDEX_V(p_map, engine_audio_maps.size(), -1);
	if (__builtin_expect(!!((p_sample) < 0 || (p_sample) >= (engine_audio_maps[p_map].samples.size())), 0)) {
		::godot ::_err_print_index_error(__FUNCTION__, "/mnt/wwn-0x50026b7782b0ee9e-part1/porter/chunkinator_2/src/vehicle/engine_sound_config.cpp", 99, p_sample, engine_audio_maps[p_map].samples.size(), "p_sample", "engine_audio_maps[p_map].samples.size()");
		return -1;
	} else
		((void)0);
	return engine_audio_maps[p_map].samples[p_sample].rpm;
}

Ref<AudioStream> LNEngineSoundConfiguration::get_map_sample_stream(int p_map, int p_sample) const {
    ERR_FAIL_INDEX_V(p_map, engine_audio_maps.size(), nullptr);
    ERR_FAIL_INDEX_V(p_sample, engine_audio_maps[p_map].samples.size(), nullptr);
    return engine_audio_maps[p_map].samples[p_sample].stream;
}

Pair<int, int> LNEngineSoundConfiguration::find_sample_range(int p_map, int p_rpm) const {
    ERR_FAIL_INDEX_V(p_map, engine_audio_maps.size(), Pair(-1, -1));
    EngineAudioSample hack = {
        .rpm = p_rpm
    };
    int bound_higher = engine_audio_maps[p_map].samples.bsearch_custom<SampleComparator>(hack, true);
    bound_higher = MIN(bound_higher, engine_audio_maps[p_map].samples.size()-1);

    return Pair(bound_higher-1, bound_higher);
}

void LNEngineSoundConfiguration::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_samples", "samples"), &LNEngineSoundConfiguration::set_samples_bind);
    ClassDB::bind_method(D_METHOD("get_samples"), &LNEngineSoundConfiguration::get_samples_bind);
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "samples"), "set_samples", "get_samples");
}
