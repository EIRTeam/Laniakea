#pragma once

#include "chunkinator/chunkinator_bounds.h"
#include "chunkinator/chunkinator_debug_drawer.h"
#include "godot_cpp/classes/ref_counted.hpp"

using namespace godot;

class ChunkinatorChunk : public RefCounted {
	Rect2i world_bounds;
	Vector2i chunk_idx;
	uint64_t generation_duration = 0;

public:
	Rect2i get_chunk_bounds() const;
	Vector2i get_chunk_index() const;
	virtual void generate() {}
	virtual void debug_draw(ChunkinatorDebugDrawer *p_debug_drawer) const {}
	friend class Chunkinator;
};
