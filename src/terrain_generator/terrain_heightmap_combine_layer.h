#pragma once

#include "chunkinator/chunkinator_chunk.h"
#include "chunkinator/chunkinator_layer.h"
#include "godot_cpp/classes/fast_noise_lite.hpp"
#include "godot_cpp/classes/image.hpp"
#include "godot_cpp/variant/packed_byte_array.hpp"
#include "profiling.h"
#include "terrain_generator/terrain_heightmap.h"
#include "terrain_generator/terrain_roads.h"

using namespace godot;

class TerrainFinalCombineLayer : public ChunkinatorLayer {
	Ref<TerrainRoadConnectionLayer> road_connection_layer;
	Ref<TerrainHeightmapLayer> heightmap_layer;
	int heightmap_size = 64;
	float height_multiplier = 0.0f;

public:
	virtual int get_chunk_size() const override;
	virtual Ref<ChunkinatorChunk> instantiate_chunk() override;
	float sample_height(const Vector2 &p_world_position) const;
	PackedByteArray sample_height_batched_bytes_f32(const Vector<Vector2> &p_world_positions) const;
	Ref<TerrainHeightmapLayer> get_heightmap_layer() const;
	Ref<TerrainRoadConnectionLayer> get_road_connection_layer() const;

	int get_heightmap_size() const { return heightmap_size; }
	void set_heightmap_size(int p_heightmap_size) { heightmap_size = p_heightmap_size; }
};

class TerrainFinalCombineChunk : public ChunkinatorChunk {
	Ref<Image> height_map;
	Ref<Image> road_sdf;
	Ref<ImageTexture> debug_draw_texture;
	Ref<ImageTexture> debug_road_sdf_texture;
	TerrainFinalCombineLayer *layer = nullptr;
	const int road_sdf_size = 128;

public:
	virtual void generate();
	virtual void debug_draw(ChunkinatorDebugDrawer *p_debug_drawer) const;
	Ref<Image> get_height_map() const;
	Ref<Image> get_road_sdf() const;
	friend class TerrainFinalCombineLayer;
};
