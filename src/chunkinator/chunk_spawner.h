#pragma once

#include "chunkinator/chunkinator.h"
#include "godot_cpp/classes/node3d.hpp"
#include "godot_cpp/templates/hash_set.hpp"

using namespace godot;

class ChunkSpawner : public Node3D {
	GDCLASS(ChunkSpawner, Node3D);

	Ref<Chunkinator> chunkinator;
	Ref<ChunkinatorLayer> layer;
	HashSet<Vector2i> spawned_chunks;

public:
	virtual void spawn_chunk(const Vector2i &p_chunk) {}
	virtual void unload_chunk(const Vector2i &p_chunk) {}
	virtual void _on_chunks_spawned() {}

	virtual StringName get_layer_name() const { return ""; }
	static void _bind_methods();
	void set_chunkinator(Ref<Chunkinator> p_chunkinator);
	Ref<Chunkinator> get_chunkinator() const;
	Ref<ChunkinatorLayer> get_layer() const;
	void _on_generation_completed();
};
