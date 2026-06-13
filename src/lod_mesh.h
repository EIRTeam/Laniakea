#pragma once

#include "godot_cpp/classes/array_mesh.hpp"

using namespace godot;

class LODMesh {
public:
	enum LODMask {
		LOD_MASK_N = 0b1,
		LOD_MASK_S = 0b10,
		LOD_MASK_E = 0b100,
		LOD_MASK_W = 0b1000
	};
	static Ref<Mesh> generate_mesh(const int p_mesh_quality);
};
