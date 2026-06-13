#pragma once

#include "chunkinator/chunkinator_chunk.h"
#include "chunkinator/chunkinator_layer.h"

class ChunkinatorGeneratorLayer : public ChunkinatorLayer {
	int chunk_size = 1024;
	int generation_radius = 4096;
	Vector2 generation_world_position;

public:
	virtual int get_chunk_size() const { return chunk_size; }
	virtual Ref<ChunkinatorChunk> instantiate_chunk();
	static Ref<ChunkinatorGeneratorLayer> create(int p_chunk_size, int p_generation_radius);
	Rect2i get_generation_rect();

	friend class Chunkinator;
};

class ChunkinatorGeneratorChunk : public ChunkinatorChunk {
};
