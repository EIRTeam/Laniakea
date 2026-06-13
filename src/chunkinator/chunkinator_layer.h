#pragma once

#include "chunkinator/chunkinator_bounds.h"
#include "chunkinator/chunkinator_chunk.h"
#include "godot_cpp/classes/ref_counted.hpp"
#include "godot_cpp/templates/local_vector.hpp"

using namespace godot;

class Chunkinator;
class ChunkinatorDebugger;

class ChunkinatorLayer : public RefCounted {
	LocalVector<Ref<ChunkinatorChunk>> chunks;
	LocalVector<Ref<ChunkinatorChunk>> generating_chunks;
	LocalVector<StringName> parents;
	LocalVector<StringName> children;
	StringName name;
	Rect2i world_rect_to_generate;
	Rect2i debug_world_rect_with_padding;
	ChunkinatorBounds chunk_idx_bounds_to_generate;
	Chunkinator *chunkinator = nullptr;
	int dag_level = 0;
	bool has_chunk_size = false;
	int chunk_size;

public:
	virtual int get_chunk_size() const = 0;
	virtual Ref<ChunkinatorChunk> instantiate_chunk() = 0;
	Ref<ChunkinatorChunk> get_chunk_by_index(Vector2i p_chunk_idx) const;
	Ref<ChunkinatorChunk> get_chunk(int p_idx) const;
	_FORCE_INLINE_ Ref<ChunkinatorChunk> get_chunk_in_position(const Vector2 &p_position) const {
		Vector2i chunk_idx = (p_position / chunk_size).floor();
		Ref<ChunkinatorChunk> chunk = get_chunk_by_index(chunk_idx);
		return chunk;
	}
	int get_chunk_count() const;
	StringName get_name() const;
	Ref<ChunkinatorLayer> get_layer(const StringName p_layer_name) const;
	void initialize_internal();
	static void _bind_methods();

	friend class Chunkinator;
	friend class ChunkinatorDebugger;
};
