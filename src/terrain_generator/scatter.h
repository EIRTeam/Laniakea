#pragma once

#include "chunkinator/chunkinator_chunk.h"
#include "chunkinator/chunkinator_layer.h"
#include "godot_cpp/classes/multi_mesh.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/variant/packed_float32_array.hpp"
#include "terrain_generator/random_point_scatter.h"
#include "terrain_generator/terrain_settings.h"

class ScatterLayer : public ChunkinatorLayer {
public:
	Ref<TerrainScatterLayerSettings> layer_settings;
	Ref<RandomPointLayer> point_layer;
	PackedFloat32Array element_probabilities;

public:
	virtual int get_chunk_size() const override;
	virtual Ref<ChunkinatorChunk> instantiate_chunk() override;
	Vector<Vector2> get_points_in_bounds(Rect2 p_world_bounds) const;
	Ref<TerrainScatterLayerSettings> get_layer_settings() const;
	Ref<RandomPointLayer> get_point_layer() { return point_layer; }

	void initialize(Ref<TerrainScatterLayerSettings> p_settings, Ref<RandomPointLayer> p_point_layer) {
		layer_settings = p_settings;
		point_layer = p_point_layer;

		DEV_ASSERT(point_layer.is_valid());
		DEV_ASSERT(layer_settings.is_valid());

		const Vector<Ref<TerrainScattererElementSettings>> &elements = p_settings->get_elements();

		element_probabilities.resize(elements.size());
		for (int i = 0; i < elements.size(); i++) {
			element_probabilities[i] = elements[i]->get_probability();
		}
	}

	PackedFloat32Array get_element_probabilities() const { return element_probabilities; }
};

class ScatterChunk : public ChunkinatorChunk {
public:
	struct ScatterElement {
		Transform3D transform;
		Ref<PackedScene> scene;
	};

private:
	LocalVector<ScatterElement> elements;
	PackedFloat32Array element_transforms;
	ScatterLayer *layer = nullptr;

	LocalVector<Ref<MultiMesh>> per_element_multimesh;

public:
	virtual void generate() override;
	const LocalVector<ScatterElement> &get_elements() const;
	const Ref<MultiMesh> get_element_multimesh(int p_idx) const;
	int get_element_multimesh_count() const;
	virtual void debug_draw(ChunkinatorDebugDrawer *p_debug_drawer) const override;
	friend class ScatterLayer;
};
