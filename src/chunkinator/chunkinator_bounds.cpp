#include "chunkinator_bounds.h"

bool ChunkinatorBounds::is_chunk_in_bounds(const Vector2i &p_chunk_idx) const {
	if (p_chunk_idx.x < min_chunk.x) {
		return false;
	}
	if (p_chunk_idx.y < min_chunk.y) {
		return false;
	}
	if (p_chunk_idx.x > max_chunk.x) {
		return false;
	}
	if (p_chunk_idx.y > max_chunk.y) {
		return false;
	}
	return true;
}
