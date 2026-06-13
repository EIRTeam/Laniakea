#pragma once

#include "chunkinator/chunk_spawner.h"
#include "eirteam_shader_rd.h"
#include "godot_cpp/classes/multi_mesh_instance3d.hpp"
#include "shaders/multimesh_lod.glsl.gen.h"
#include "terrain_generator/terrain_settings.h"

class ScatterManager : public ChunkSpawner {
	struct ScatterChunkElementInstance {
		MultiMeshInstance3D *multimesh = nullptr;
		std::vector<Ref<TerrainScattererLODMesh>> sorted_lod_meshes;
	};

	struct ScatterChunkData {
		Node3D *root_node = nullptr;
		Vector<ScatterChunkElementInstance> element_instances;
	};
	HashMap<Vector2i, ScatterChunkData> scatter_chunks;

	virtual void spawn_chunk(const Vector2i &p_chunk);
	virtual void unload_chunk(const Vector2i &p_chunk);
	virtual StringName get_layer_name() const { return "Trees"; }
	void _update_chunk_meshes();
};
