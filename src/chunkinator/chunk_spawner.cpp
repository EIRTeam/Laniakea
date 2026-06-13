#include "chunk_spawner.h"

#include "chunkinator/chunkinator_chunk.h"
#include "godot_cpp/templates/hash_set.hpp"

void ChunkSpawner::_bind_methods() {
}

void ChunkSpawner::set_chunkinator(Ref<Chunkinator> p_chunkinator) {
	chunkinator = p_chunkinator;
	DEV_ASSERT(chunkinator.is_valid());
	print_line("LAYER NAME: ", get_layer_name());
	layer = chunkinator->get_layer(get_layer_name());
	DEV_ASSERT(layer.is_valid());
	chunkinator->connect("generation_completed", callable_mp(this, &ChunkSpawner::_on_generation_completed));
}

Ref<Chunkinator> ChunkSpawner::get_chunkinator() const {
	return chunkinator;
}

Ref<ChunkinatorLayer> ChunkSpawner::get_layer() const {
	return layer;
}

void ChunkSpawner::_on_generation_completed() {
	HashSet<Vector2i> new_chunks;
	// Add new superchunks
	for (int i = 0; i < layer->get_chunk_count(); i++) {
		Ref<ChunkinatorChunk> chunk = layer->get_chunk(i);
		const Vector2i chunk_idx = chunk->get_chunk_index();
		new_chunks.insert(chunk->get_chunk_index());
		if (!spawned_chunks.has(chunk_idx)) {
			// Add new superchunk to tree
			spawned_chunks.insert(chunk_idx);
			spawn_chunk(chunk_idx);
		}
	}
	// Delete existing ones that don't matter anymore
	HashSet<Vector2i>::Iterator it = spawned_chunks.begin();
	while (it != spawned_chunks.end()) {
		if (!new_chunks.has(*it)) {
			const Vector2i chunk_idx = *it;
			unload_chunk(chunk_idx);
			++it;
			spawned_chunks.erase(chunk_idx);
			continue;
		}
		++it;
	}

	_on_chunks_spawned();
}
