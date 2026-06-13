#include "scatter.h"

#include "godot_cpp/classes/random_number_generator.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/templates/hash_map.hpp"
#include "godot_cpp/templates/hashfuncs.hpp"
#include "godot_cpp/variant/packed_float32_array.hpp"
#include "godot_cpp/variant/transform3d.hpp"
#include "terrain_generator/random_point_scatter.h"
#include "terrain_generator/terrain_heightmap.h"
#include "terrain_generator/terrain_settings.h"

int ScatterLayer::get_chunk_size() const {
	return layer_settings->get_layer_chunk_size();
}

Ref<ChunkinatorChunk> ScatterLayer::instantiate_chunk() {
	Ref<ScatterChunk> chunk;
	chunk.instantiate();
	chunk->layer = this;
	return chunk;
}

Ref<TerrainScatterLayerSettings> ScatterLayer::get_layer_settings() const {
	return layer_settings;
}

void ScatterChunk::generate() {
	Ref<TerrainScatterLayerSettings> settings = layer->get_layer_settings();

	Ref<RandomPointLayer> point_layer = layer->get_point_layer();
	Vector<Vector2> points = point_layer->get_points_in_bounds(get_chunk_bounds());

	int64_t seed = HashMapHasherDefault::hash(Vector3i(get_chunk_index().x, get_chunk_index().y, 0));

	Ref<RandomNumberGenerator> rng;
	rng.instantiate();
	rng->set_seed(seed);

	const PackedFloat32Array element_probabilities = layer->get_element_probabilities();

	const float none_probability = settings->get_none_probability();

	const Vector<Ref<TerrainScattererElementSettings>> &element_settings = settings->get_elements();

	Ref<TerrainHeightmapLayer> heightmap_layer = layer->get_layer("Heightmap Layer");

	elements.reserve(points.size());

	LocalVector<LocalVector<int>> per_element_transforms;

	per_element_transforms.resize(element_settings.size());

	for (int i = 0; i < points.size(); i++) {
		if (rng->randf() < none_probability) {
			continue;
		}

		const int element_idx = rng->rand_weighted(element_probabilities);

		Transform3D element_trf;
		const float height = heightmap_layer->sample_noise(points[i]);
		element_trf.origin = Vector3(points[i].x, height, points[i].y);

		per_element_transforms[element_idx].push_back(elements.size());

		elements.push_back({
				.transform = element_trf,
				.scene = element_settings[element_idx]->get_scene(),
		});
	}

	per_element_multimesh.clear();
	per_element_multimesh.resize(element_settings.size());

	for (int i = 0; i < element_settings.size(); i++) {
		if (per_element_transforms[i].is_empty()) {
			continue;
		}

		per_element_multimesh[i].instantiate();
		per_element_multimesh[i]->set_mesh(element_settings[i]->get_mesh(0));
		per_element_multimesh[i]->set_transform_format(MultiMesh::TRANSFORM_3D);
		per_element_multimesh[i]->set_instance_count(per_element_transforms[i].size());

		for (int j = 0; j < per_element_transforms[i].size(); j++) {
			per_element_multimesh[i]->set_instance_transform(j, elements[per_element_transforms[i][j]].transform);
		}
	}
}

const LocalVector<ScatterChunk::ScatterElement> &ScatterChunk::get_elements() const {
	return elements;
}

const Ref<MultiMesh> ScatterChunk::get_element_multimesh(int p_idx) const {
	ERR_FAIL_INDEX_V(p_idx, per_element_multimesh.size(), nullptr);
	return per_element_multimesh[p_idx];
}

int ScatterChunk::get_element_multimesh_count() const {
	return per_element_multimesh.size();
}

void ScatterChunk::debug_draw(ChunkinatorDebugDrawer *p_debug_drawer) const {
	for (int i = 0; i < elements.size(); i++) {
		const Vector2 pos = Vector2(elements[i].transform.origin.x, elements[i].transform.origin.z);
		p_debug_drawer->draw_circle(pos, 2.0f);
	}
}
