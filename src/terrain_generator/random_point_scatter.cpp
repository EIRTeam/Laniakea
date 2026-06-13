#include "random_point_scatter.h"

#include "godot_cpp/classes/random_number_generator.hpp"
#include "godot_cpp/core/print_string.hpp"
#include "godot_cpp/templates/hashfuncs.hpp"

int RandomPointLayer::get_chunk_size() const {
	return generation_settings.chunk_size;
}

Ref<ChunkinatorChunk> RandomPointLayer::instantiate_chunk() {
	Ref<RandomPointChunk> chunk;
	chunk.instantiate();
	chunk->generation_settings = generation_settings;
	return chunk;
}

Vector<Vector2> RandomPointLayer::get_points_in_bounds(Rect2 p_world_bounds) const {
	Vector<Vector2> points;
	for (int i = 0; i < get_chunk_count(); i++) {
		Ref<RandomPointChunk> chunk = get_chunk(i);
		if (!chunk->get_chunk_bounds().intersects(p_world_bounds)) {
			continue;
		}

		for (Vector2 point : chunk->points) {
			if (p_world_bounds.has_point(point)) {
				points.push_back(point);
			}
		}
	}
	return points;
}

void RandomPointLayer::set_settings(const RandomPointGenerationSettings &p_settings) {
	generation_settings = p_settings;
}

void RandomPointChunk::generate() {
	const Vector2i grid_side_count = generation_settings.grid_element_count;
	const Vector2 side_size = Vector2(get_chunk_bounds().size) / Vector2(grid_side_count);
	const Vector2 half_side_size = side_size / 2.0f;
	const Vector2i chunk_idx = get_chunk_index();

	int64_t seed = HashMapHasherDefault::hash(Vector3i(chunk_idx.x, chunk_idx.y, generation_settings.seed));

	Ref<RandomNumberGenerator> rng;
	rng.instantiate();
	rng->set_seed(seed);

	for (int x = 0; x < grid_side_count.x; x++) {
		for (int y = 0; y < grid_side_count.x; y++) {
			const Vector2 cell_center = get_chunk_bounds().position + Vector2(x * side_size.x, y * side_size.y) + Vector2(half_side_size.y, half_side_size.y);
			const float x_jitter = generation_settings.jitter_factor * half_side_size.x * rng->randf();
			const float y_jitter = generation_settings.jitter_factor * half_side_size.y * rng->randf();
			points.push_back(cell_center + Vector2(x_jitter, y_jitter));
		}
	}
}

void RandomPointChunk::debug_draw(ChunkinatorDebugDrawer *p_debug_drawer) const {
	for (Vector2 p : points) {
		p_debug_drawer->draw_circle(p, 2.0f);
	}
}
