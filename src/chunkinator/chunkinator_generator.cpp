#include "chunkinator_generator.h"

Ref<ChunkinatorChunk> ChunkinatorGeneratorLayer::instantiate_chunk() {
	Ref<ChunkinatorGeneratorChunk> chunk;
	chunk.instantiate();
	return chunk;
}

Ref<ChunkinatorGeneratorLayer> ChunkinatorGeneratorLayer::create(int p_chunk_size, int p_generation_radius) {
	Ref<ChunkinatorGeneratorLayer> layer;
	layer.instantiate();
	layer->chunk_size = p_chunk_size;
	layer->generation_radius = p_generation_radius;
	return layer;
}

Rect2i ChunkinatorGeneratorLayer::get_generation_rect() {
	Rect2 new_generation_rect = Rect2(-generation_radius, -generation_radius, generation_radius * 2, generation_radius * 2);
	new_generation_rect.position += generation_world_position;
	return new_generation_rect;
}
