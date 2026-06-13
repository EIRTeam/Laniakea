#pragma once

#include "godot_cpp/variant/vector2i.hpp"

using namespace godot;

struct ChunkinatorBounds {
	Vector2i min_chunk;
	Vector2i max_chunk;

	bool is_chunk_in_bounds(const Vector2i &p_chunk_idx) const;
};
