#pragma once

#include "chunkinator/chunkinator_layer.h"
#include "godot_cpp/classes/node3d.hpp"

using namespace godot;

/*
double WorldMan::sample_superchunk_noise(const WorldMan::WorldManSuperchunk &p_superchunk, const Vector2 &p_local_position) const {
	const Vector2 pos = Vector2(p_superchunk.position * SUPERCHUNK_SIZE) + p_local_position;
	return noise->get_noise_2dv(pos * 0.01) * 500.0;
}*/

class TerrainGenerator : public Node3D {
	GDCLASS(TerrainGenerator, Node3D);
	Ref<Chunkinator> chunkinator;

public:
	void update(Vector3 p_camera_position);
};
