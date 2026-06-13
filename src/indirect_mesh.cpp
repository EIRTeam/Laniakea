#include "indirect_mesh.h"

#include "bind_macros.h"
#include "godot_cpp/classes/array_mesh.hpp"
#include "godot_cpp/classes/material.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/core/print_string.hpp"

#include <optional>

void IndirectMesh::lod_set_lod_begin_distance(int p_idx, float p_lod_begin_instance) {
	ERR_FAIL_INDEX(p_idx, lods.size());
	lods.write[p_idx].lod_begin_distance = p_lod_begin_instance;
}

float IndirectMesh::lod_get_lod_begin_distance(int p_idx) const {
	ERR_FAIL_INDEX_V(p_idx, lods.size(), 0.0f);
	return lods[p_idx].lod_begin_distance;
}

void IndirectMesh::lod_set_mesh(int p_idx, Ref<Mesh> p_mesh) {
	ERR_FAIL_INDEX(p_idx, lods.size());
	lods.write[p_idx].mesh = p_mesh;
}

Ref<ArrayMesh> IndirectMesh::lod_get_mesh(int p_idx) const {
	ERR_FAIL_INDEX_V(p_idx, lods.size(), nullptr);
	return lods[p_idx].mesh;
}

Ref<Mesh> IndirectMesh::build_combined_mesh() const {
	if (!combined_mesh.is_valid()) {
		Ref<ArrayMesh> mesh;
		mesh.instantiate();

		std::optional<BitField<Mesh::ArrayFormat>> format;
		float radius = 0.0f;

		for (int i = 0; i < get_lod_count(); i++) {
			Ref<ArrayMesh> lod_mesh = lod_get_mesh(i);
			for (int surface_idx = 0; surface_idx < lod_mesh->get_surface_count(); surface_idx++) {
				BitField<Mesh::ArrayFormat> format = lod_mesh->surface_get_format(surface_idx);
				Mesh::PrimitiveType primitive = lod_mesh->surface_get_primitive_type(surface_idx);

				Array data = lod_mesh->surface_get_arrays(surface_idx);
				mesh->add_surface_from_arrays(primitive, data, Array(), Dictionary(), format);
				mesh->surface_set_material(mesh->get_surface_count() - 1, lod_mesh->surface_get_material(surface_idx));
			}
			AABB aabb = lod_mesh->get_aabb();
			radius = MAX(radius, aabb.get_center().length() + aabb.get_longest_axis_size());
		}

		const_cast<IndirectMesh *>(this)->mesh_radius = radius;
		const_cast<IndirectMesh *>(this)->combined_mesh = mesh;
	}

	return combined_mesh;
}

void IndirectMesh::_get_property_list(List<PropertyInfo> *p_list) const {
	for (int i = 0; i < lods.size(); i++) {
		p_list->push_back(PropertyInfo(Variant::FLOAT, "lod_" + itos(i) + "/lod_begin_distance", PROPERTY_HINT_RANGE, "0,100,or_greater", PROPERTY_USAGE_EDITOR));
		p_list->push_back(PropertyInfo(Variant::OBJECT, "lod_" + itos(i) + "/mesh", PROPERTY_HINT_RESOURCE_TYPE, "ArrayMesh", PROPERTY_USAGE_EDITOR));
	}
}

bool IndirectMesh::_get(const StringName &p_name, Variant &r_ret) const {
	String sname = p_name;
	if (sname.begins_with("lod_")) {
		print_line("REQ", sname);
		int sl = sname.find("/");
		if (sl == -1) {
			return false;
		}
		int idx = sname.substr(4, sl - 4).to_int();
		String what = sname.get_slicec('/', 1);
		if (what == "mesh") {
			r_ret = lod_get_mesh(idx);
		} else if (what == "lod_begin_distance") {
			r_ret = lod_get_lod_begin_distance(idx);
		}

		print_line(idx, r_ret);
		return true;
	}

	return false;
}

bool IndirectMesh::_set(const StringName &p_name, const Variant &p_value) {
	String sname = p_name;

	if (sname.begins_with("lod_")) {
		int sl = sname.find("/");
		if (sl == -1) {
			return false;
		}
		int idx = sname.substr(4, sl - 4).to_int();

		String what = sname.get_slicec('/', 1);
		if (what == "mesh") {
			lod_set_mesh(idx, p_value);
		} else if (what == "lod_begin_distance") {
			lod_set_lod_begin_distance(idx, p_value);
		}
		return true;
	}

	return false;
}

Array IndirectMesh::_get_lods() const {
	Array ret;

	for (int i = 0; i < lods.size(); i++) {
		Dictionary data;
		data["mesh"] = lods[i].mesh;
		data["lod_begin_distance"] = lods[i].lod_begin_distance;
		ret.push_back(data);
	}

	return ret;
}

void IndirectMesh::_set_lods(const Array &p_lods) {
	lods.clear();
	for (int i = 0; i < p_lods.size(); i++) {
		Dictionary d = p_lods[i];
		IndirectMeshLOD lod = {
			.mesh = d.get("mesh", Variant()),
			.lod_begin_distance = d.get("lod_begin_distance", 0.0f),
		};

		lods.push_back(lod);
	}
}

void IndirectMesh::_bind_methods() {
	MAKE_BIND_INT(IndirectMesh, lod_count);

	ClassDB::bind_method(D_METHOD("_set_lods", "lods"), &IndirectMesh::_set_lods);
	ClassDB::bind_method(D_METHOD("_get_lods"), &IndirectMesh::_get_lods);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "_lods", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_INTERNAL), "_set_lods", "_get_lods");
}

int IndirectMesh::get_lod_count() const {
	return lods.size();
}

void IndirectMesh::set_lod_count(int p_lod_count) {
	lods.resize(p_lod_count);
	notify_property_list_changed();
}

int IndirectMesh::get_total_surface_count() const {
	int ret = 0;

	for (const IndirectMeshLOD &lod : lods) {
		ret += lod.mesh.is_valid() ? lod.mesh->get_surface_count() : 0;
	}

	return ret;
}
