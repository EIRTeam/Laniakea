#pragma once

#include "godot_cpp/classes/array_mesh.hpp"
#include "godot_cpp/classes/mesh.hpp"
#include "godot_cpp/classes/resource.hpp"
#include "godot_cpp/core/property_info.hpp"

using namespace godot;

class IndirectMesh : public Resource {
	GDCLASS(IndirectMesh, Resource);

	int MAX_SURFACES_PER_LOD = 4;

	struct IndirectMeshLOD {
		Ref<ArrayMesh> mesh;
		float lod_begin_distance = 0.0f;
	};

	float mesh_radius = 0.0f;

	Vector<IndirectMeshLOD> lods;

	void _get_property_list(List<PropertyInfo> *p_list) const;
	bool _get(const StringName &p_name, Variant &r_ret) const;
	bool _set(const StringName &p_name, const Variant &p_value);
	Array _get_lods() const;
	void _set_lods(const Array &p_lods);
	static void _bind_methods();

	Ref<ArrayMesh> combined_mesh;

public:
	int get_lod_count() const;
	void set_lod_count(int p_lod_count);
	int get_total_surface_count() const;
	void lod_set_lod_begin_distance(int p_idx, float p_lod_begin_distance);
	float lod_get_lod_begin_distance(int p_idx) const;
	void lod_set_mesh(int p_idx, Ref<Mesh> p_mesh);
	Ref<ArrayMesh> lod_get_mesh(int p_idx) const;
	Ref<Mesh> build_combined_mesh() const;

	float get_mesh_radius() const { return mesh_radius; }
	void set_mesh_radius(float p_mesh_radius) { mesh_radius = p_mesh_radius; }
};
