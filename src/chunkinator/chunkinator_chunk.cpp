#include "chunkinator_chunk.h"

Rect2i ChunkinatorChunk::get_chunk_bounds() const {
	return world_bounds;
}

Vector2i ChunkinatorChunk::get_chunk_index() const {
	return chunk_idx;
}
