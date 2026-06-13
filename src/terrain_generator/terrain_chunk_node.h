#pragma once

#include "godot_cpp/classes/collision_shape3d.hpp"
#include "godot_cpp/classes/image_texture.hpp"
#include "godot_cpp/classes/mesh_instance3d.hpp"
#include "godot_cpp/classes/node3d.hpp"
#include "godot_cpp/classes/shader_material.hpp"
#include "godot_cpp/classes/shape3d.hpp"
#include "godot_cpp/classes/static_body3d.hpp"
#include "godot_cpp/classes/texture2d_array.hpp"
#include "godot_cpp/classes/worker_thread_pool.hpp"
#include "godot_cpp/templates/hash_map.hpp"
#include "lod_mesh.h"
#include "quadtree.h"
#include "terrain_generator/texture_array_queue.h"

using namespace godot;

class TerrainChunkNode : public Node3D {
	enum ChunkStatus {
		DONE,
		NEEDS_UNLOADNG,
		NEEDS_INSTANCING
	};

	struct TerrainChunkInformation {
		MeshInstance3D *mesh_instance = nullptr;
		Ref<Shape3D> shape;
		StaticBody3D *body = nullptr;
		CollisionShape3D *collision_shape = nullptr;
		ChunkStatus status = ChunkStatus::NEEDS_INSTANCING;
		bool lod_mask_needs_updating = true;
		BitField<LODMesh::LODMask> lod_mask = 0;
		int lod_depth = 0;
	};

	struct QuadTreeUpdateTask {
		WorkerThreadPool::TaskID quadtree_generation_task_id = -1;
		LocalVector<Rect2i> chunks_to_unload;
		bool has_camera_position = false;
		Vector2i camera_position;
	};

	struct CollisionMeshGenerationTask {
		WorkerThreadPool::GroupID task_id = -1;
		LocalVector<Rect2i> chunks_that_need_collision_mesh;
	};

	QuadTreeUpdateTask current_quadtree_task;
	CollisionMeshGenerationTask current_mesh_generation_task;

	enum UpdateStatus {
		IDLING,
		GENERATING_QUADTREE,
		GENERATING_COLLISION_MESH
	};

	UpdateStatus status = UpdateStatus::IDLING;

	HashMap<Rect2i, TerrainChunkInformation> chunks;

	Ref<ShaderMaterial> material;
	Ref<Image> heightmap;
	Ref<ImageTexture> heightmap_texture;
	Ref<ImageTexture> road_sdf_texture;
	Ref<Mesh> plane_mesh;
	Rect2i world_rect;
	TextureArrayQueue::TextureHandle heightmap_texture_handle;
	Ref<Texture2D> heightmap_spatial_page_texture;
	Ref<TextureLayered> heightmap_texture_array;
	int mesh_resolution;
	int collision_mesh_resolution;

	int superchunk_size = 0;
	float lod_threshold_multiplier = 0.0f;
	void _update_quadtree_task();
	void _generate_collision_meshes_task(uint32_t p_idx);
	void _check_quadtree_generation();
	void _check_collision_mesh_generation();
	void _unload_queued_chunks();
	void _update_chunk_lod_masks();

public:
	struct TerrainChunkNodeInitializationProperties {
		int superchunk_size;
		Vector2i chunk_idx;
		Ref<Image> heightmap;
		Ref<ImageTexture> height_texture;
		Ref<ImageTexture> road_sdf_texture;
		Ref<Mesh> plane_mesh;
		Ref<ShaderMaterial> material;
		TextureArrayQueue::TextureHandle heightmap_texture_handle;
		Ref<Texture2D> heightmap_spatial_page_texture;
		Ref<TextureLayered> heightmap_texture_array;
		int mesh_resolution = 17;
		int collision_mesh_resolution = 17;
		float lod_threshold_multiplier = 1.0f;
	};
	void initialize(const TerrainChunkNodeInitializationProperties &p_init_properties);
	void update(Vector2i p_camera_position);
	TextureArrayQueue::TextureHandle get_heightmap_texture_handle() const;
	TerrainChunkNode();
};
