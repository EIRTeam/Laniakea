#include "scatter_manager.h"

#include "godot_cpp/classes/multi_mesh_instance3d.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "terrain_generator/scatter.h"
#include "terrain_generator/terrain_settings.h"

#include <algorithm>

void ScatterManager::spawn_chunk(const Vector2i &p_chunk) {
	Ref<ScatterLayer> scatter_layer = get_layer();
	Ref<ScatterChunk> chunk = scatter_layer->get_chunk_by_index(p_chunk);
	DEV_ASSERT(chunk.is_valid());

	Node3D *root = memnew(Node3D);

	Vector<Ref<TerrainScattererElementSettings>> all_element_settings = scatter_layer->get_layer_settings()->get_elements();

	ScatterChunkData chunk_data = {
		.root_node = root,
	};

	for (int element_i = 0; element_i < chunk->get_element_multimesh_count(); element_i++) {
		Ref<MultiMesh> multimesh = chunk->get_element_multimesh(element_i);

		Ref<TerrainScattererElementSettings> element_settings = all_element_settings[element_i];

		std::vector<Ref<TerrainScattererLODMesh>> meshes;

		for (int mesh_i = 0; mesh_i < element_settings->get_mesh_count(); mesh_i++) {
			meshes.push_back(element_settings->get_mesh(mesh_i));
			print_line("BEGIN", meshes[mesh_i]->get_mesh()->get_path());
		}

		// Sort by descending order
		std::sort(meshes.begin(), meshes.end(), [](const Ref<TerrainScattererLODMesh> &p_a, const Ref<TerrainScattererLODMesh> &p_b) {
			return p_a->get_begin_distance() > p_b->get_begin_distance();
		});

		MultiMeshInstance3D *mm = memnew(MultiMeshInstance3D);
		mm->set_multimesh(multimesh);
		root->add_child(mm);

		chunk_data.element_instances.push_back({
				.multimesh = mm,
				.sorted_lod_meshes = meshes,
		});
	}

	//for (const ScatterChunk::ScatterElement &element : elements) {
	/*
	Node *scene_instance = element.scene->instantiate();
	Node3D *scene_instance_node_3d = Object::cast_to<Node3D>(scene_instance);
	if (scene_instance_node_3d == nullptr) {
		ERR_FAIL_MSG(vformat("Scatter scene %s was not of type Node3D!", element.scene->get_path()));
		memdelete(root);
		memdelete(scene_instance);
		return;
	}

	root->add_child(scene_instance_node_3d);
	scene_instance_node_3d->set_transform(element.transform)*/
	//}
	add_child(root);

	scatter_chunks.insert(p_chunk, chunk_data);
}

void ScatterManager::unload_chunk(const Vector2i &p_chunk) {
	DEV_ASSERT(scatter_chunks.has(p_chunk));
	memdelete(scatter_chunks[p_chunk].root_node);
	scatter_chunks.erase(p_chunk);
}

void ScatterManager::_update_chunk_meshes() {
	/*
	Ref<ScatterLayer> scatter_layer = get_layer();
	const int chunk_size = scatter_layer->get_layer_settings()->get_layer_chunk_size();
	for (KeyValue<Vector2i, ScatterChunkData> &kv : scatter_chunks) {
		Rect2 chunk_rect = Rect2(kv.key * Vector2(chunk_size, chunk_size), Vector2(chunk_size, chunk_size));
		for (ScatterChunkElementInstance &instance : kv.value.element_instances) {
			chunk_rect.distance_to()

			for (std::vector<Ref<TerrainScattererLODMesh>>::reverse_iterator it = instance.sorted_lod_meshes.rbegin(); it != instance.sorted_lod_meshes.rend(); it++) {
			}
		}
	*/
}
