#include "terrain_settings.h"

#include "bind_macros.h"
#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/core/error_macros.hpp"

void TerrainSettings::set_height_noise_layers_bind(TypedArray<TerrainHeightNoiseLayerSettings> p_noise_layers) {
	height_noise_layers.clear();
	height_noise_layers.resize(p_noise_layers.size());

	Ref<TerrainHeightNoiseLayerSettings> *height_noise_layers_w = height_noise_layers.ptrw();

	for (int i = 0; i < p_noise_layers.size(); i++) {
		height_noise_layers_w[i] = p_noise_layers[i];
	}
}

void TerrainSettings::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_height_noise_layers", "curve"), &TerrainSettings::set_height_noise_layers_bind);
	ClassDB::bind_method(D_METHOD("get_height_noise_layers"), &TerrainSettings::get_height_noise_layers_bind);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "height_noise_layers", PROPERTY_HINT_ARRAY_TYPE, "TerrainHeightNoiseLayerSettings"), "set_height_noise_layers", "get_height_noise_layers");

	MAKE_BIND_RESOURCE(TerrainSettings, terrain_material, ShaderMaterial);

	MAKE_BIND_FLOAT(TerrainSettings, lod_radius_threshold_multiplier);
	MAKE_BIND_INT(TerrainSettings, mesh_quality);
	MAKE_BIND_INT(TerrainSettings, collision_mesh_quality);
	MAKE_BIND_INT(TerrainSettings, terrain_generation_radius);
	MAKE_BIND_INT(TerrainSettings, scatter_generation_radius);

	ClassDB::bind_method(D_METHOD("set_scatter_layers", "scatter_layers_bind"), &TerrainSettings::set_scatter_layers_bind);
	ClassDB::bind_method(D_METHOD("get_scatter_layers"), &TerrainSettings::get_scatter_layers_bind);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "scatter_layers", PROPERTY_HINT_ARRAY_TYPE, "TerrainScatterLayerSettings"), "set_scatter_layers", "get_scatter_layers");
}

int TerrainSettings::get_scatter_layer_count() const {
	return scatter_layers.size();
}

Ref<TerrainScatterLayerSettings> TerrainSettings::get_scatter_layer(int p_idx) const {
	ERR_FAIL_INDEX_V(p_idx, scatter_layers.size(), nullptr);
	return scatter_layers[p_idx];
}

TypedArray<TerrainHeightNoiseLayerSettings> TerrainSettings::get_height_noise_layers_bind() const {
	TypedArray<TerrainHeightNoiseLayerSettings> out;
	out.resize(height_noise_layers.size());

	for (int i = 0; i < height_noise_layers.size(); i++) {
		out[i] = height_noise_layers[i];
	}

	return out;
}

void TerrainSettings::set_scatter_layers_bind(TypedArray<TerrainScatterLayerSettings> p_scatter_layers) {
	scatter_layers.clear();
	scatter_layers.resize(p_scatter_layers.size());

	Ref<TerrainScatterLayerSettings> *scatter_layers_w = scatter_layers.ptrw();

	for (int i = 0; i < p_scatter_layers.size(); i++) {
		scatter_layers_w[i] = p_scatter_layers[i];
	}
}

TypedArray<TerrainScatterLayerSettings> TerrainSettings::get_scatter_layers_bind() const {
	TypedArray<TerrainScatterLayerSettings> out;
	out.resize(scatter_layers.size());

	for (int i = 0; i < scatter_layers.size(); i++) {
		out[i] = scatter_layers[i];
	}

	return out;
}

void TerrainSettings::set_terrain_material(Ref<ShaderMaterial> p_material) {
	terrain_material = p_material;
}

Ref<ShaderMaterial> TerrainSettings::get_terrain_material() const {
	return terrain_material;
}

int TerrainSettings::get_height_layer_count() const {
	return height_noise_layers.size();
}

Ref<TerrainHeightNoiseLayerSettings> TerrainSettings::get_height_layer(int p_idx) const {
	ERR_FAIL_INDEX_V(p_idx, height_noise_layers.size(), nullptr);
	return height_noise_layers[p_idx];
}

void TerrainHeightNoiseLayerSettings::_bind_methods() {
	MAKE_BIND_RESOURCE(TerrainHeightNoiseLayerSettings, curve, Curve);
	MAKE_BIND_RESOURCE(TerrainHeightNoiseLayerSettings, noise, Noise);
}

void TerrainScattererElementSettings::_bind_methods() {
	MAKE_BIND_FLOAT(TerrainScattererElementSettings, probability);
	MAKE_BIND_FLOAT(TerrainScattererElementSettings, impostor_begin_distance);
	MAKE_BIND_FLOAT(TerrainScattererElementSettings, impostor_margin_distance);
	MAKE_BIND_RESOURCE(TerrainScattererElementSettings, scene, PackedScene);
	MAKE_BIND_FLOAT32_ARRAY(TerrainScattererElementSettings, lod_distances);

	ClassDB::bind_method(D_METHOD("set_meshes", "meshes"), &TerrainScattererElementSettings::set_meshes_bind);
	ClassDB::bind_method(D_METHOD("get_meshes"), &TerrainScattererElementSettings::get_meshes_bind);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "meshes", PROPERTY_HINT_ARRAY_TYPE, "TerrainScattererLODMesh"), "set_meshes", "get_meshes");
}

TypedArray<TerrainScattererLODMesh> TerrainScattererElementSettings::get_meshes_bind() const {
	TypedArray<TerrainScattererLODMesh> out;
	out.resize(meshes.size());

	for (int i = 0; i < meshes.size(); i++) {
		out[i] = meshes[i];
	}

	return out;
}

int TerrainScattererElementSettings::get_mesh_count() {
	return meshes.size();
}

Ref<TerrainScattererLODMesh> TerrainScattererElementSettings::get_mesh(int p_idx) const {
	ERR_FAIL_INDEX_V(p_idx, meshes.size(), nullptr);
	return meshes[p_idx];
}

void TerrainScattererElementSettings::set_meshes_bind(TypedArray<TerrainScattererLODMesh> p_meshes) {
	meshes.resize(p_meshes.size());

	Ref<TerrainScattererLODMesh> *elements_w = meshes.ptr();

	for (int i = 0; i < p_meshes.size(); i++) {
		elements_w[i] = p_meshes[i];
	}
}

TypedArray<TerrainScattererElementSettings> TerrainScatterLayerSettings::get_elements_bind() const {
	TypedArray<TerrainScattererElementSettings> out;
	out.resize(elements.size());

	for (int i = 0; i < elements.size(); i++) {
		out[i] = elements[i];
	}

	return out;
}

void TerrainScatterLayerSettings::set_elements_bind(const TypedArray<TerrainScattererElementSettings> &p_elements) {
	elements.resize(p_elements.size());

	Ref<TerrainScattererElementSettings> *elements_w = elements.ptrw();

	for (int i = 0; i < elements.size(); i++) {
		elements_w[i] = p_elements[i];
	}
}

void TerrainScatterLayerSettings::_bind_methods() {
	MAKE_BIND_FLOAT(TerrainScatterLayerSettings, none_probability);
	MAKE_BIND_INT(TerrainScatterLayerSettings, layer_chunk_size);
	MAKE_BIND_INT(TerrainScatterLayerSettings, layer_element_count_per_side);
	MAKE_BIND_STRING_NAME(TerrainScatterLayerSettings, debug_name);

	ClassDB::bind_method(D_METHOD("set_elements", "elements"), &TerrainScatterLayerSettings::set_elements_bind);
	ClassDB::bind_method(D_METHOD("get_elements"), &TerrainScatterLayerSettings::get_elements_bind);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "elements", PROPERTY_HINT_ARRAY_TYPE, "TerrainScattererElementSettings"), "set_elements", "get_elements");
}

void TerrainScattererLODMesh::_bind_methods() {
	MAKE_BIND_RESOURCE(TerrainScattererLODMesh, mesh, Mesh);
	MAKE_BIND_FLOAT(TerrainScattererLODMesh, begin_distance);
}
