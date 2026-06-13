#include "terrain_roads.h"

#include "godot_cpp/classes/a_star_grid2d.hpp"
#include "godot_cpp/core/math.hpp"
#include "godot_cpp/core/print_string.hpp"
#include "godot_cpp/templates/hash_map.hpp"
#include "godot_cpp/templates/local_vector.hpp"
#include "profiling.h"
#include "segment_quadtree.h"
#include "terrain_generator/random_point_scatter.h"
#include "terrain_generator/terrain_heightmap.h"
#include "tracy/Tracy.hpp"

#include <limits>

int TerrainRoadConnectionLayer::get_chunk_size() const {
	return 16384;
}

Ref<ChunkinatorChunk> TerrainRoadConnectionLayer::instantiate_chunk() {
	Ref<TerrainRoadConnectionChunk> chunk;
	chunk.instantiate();
	chunk->layer = this;
	return chunk;
}

Vector<Vector2> TerrainRoadConnectionLayer::get_points_in_radius(Vector2 p_position, float p_radius) const {
	ZoneScoped;
	Ref<RandomPointLayer> layer = get_layer("Road Random Points");
	Vector<Vector2> points = layer->get_points_in_bounds(Rect2(p_position - Vector2(p_radius, p_radius), Vector2(p_radius, p_radius)));
	const float radius_squared = p_radius * p_radius;
	for (int i = points.size() - 1; i >= 0; i--) {
		if (points[i].distance_squared_to(p_position) > radius_squared) {
			points.remove_at(i);
		}
	}
	return points;
}

Vector<Vector2> TerrainRoadConnectionLayer::get_points_in_bounds(Rect2 p_bounds) const {
	Ref<RandomPointLayer> layer = get_layer("Road Random Points");
	return layer->get_points_in_bounds(p_bounds);
}

double TerrainRoadConnectionLayer::get_height_at_position(Vector2 p_position) const {
	Ref<TerrainHeightmapLayer> layer = get_layer("Heightmap Layer");
	return layer->sample_height(p_position);
}

struct PointSorter {
	struct PointDistance {
		int idx;
		float distance;
	};
	_FORCE_INLINE_ bool operator()(const PointDistance &l, const PointDistance &r) const {
		return l.distance < r.distance;
	}
};

void sort_by_dist_to_point(Vector2 p_point, Vector<Vector2> &ri_points) {
	FuncProfile;
	LocalVector<PointSorter::PointDistance> distances_squared;
	distances_squared.reserve(ri_points.size());

	for (int i = 0; i < ri_points.size(); i++) {
		distances_squared.push_back({
				.idx = i,
				.distance = ri_points[i].distance_squared_to(p_point),
		});
	}

	distances_squared.sort_custom<PointSorter>();
	Vector<Vector2> positions;
	positions.resize(ri_points.size());
	Vector2 *positions_ptrw = positions.ptrw();

	for (int i = 0; i < ri_points.size(); i++) {
		positions_ptrw[i] = ri_points[distances_squared[i].idx];
	}

	ri_points = positions;
}

bool TerrainRoadConnectionLayer::get_distance_to_closest_road_segment(const Vector2 &p_position, SegmentQuadtree::QuadTreeSegment &r_segment, float *r_distance, Vector2 *r_closest_point) const {
	Ref<TerrainRoadConnectionChunk> chunk = get_chunk_in_position(p_position);
	return chunk->get_distance_to_closest_road_segment(p_position, r_segment, r_distance, r_closest_point);
}

Vector<TerrainRoadConnectionLayer::SegmentQueryResult> TerrainRoadConnectionLayer::get_distance_to_closest_road_segments_batched(const Vector<Vector2> &p_positions) const {
	FuncProfile;
	Vector<SegmentQueryResult> out;
	out.resize(p_positions.size());
	SegmentQueryResult *out_ptrw = out.ptrw();

	HashMap<Vector2i, Vector<Vector2>> buckets;

	for (int i = 0; i < p_positions.size(); i++) {
		Ref<TerrainRoadConnectionChunk> chunk = get_chunk_in_position(p_positions[i]);
		SegmentQuadtree::QuadTreeSegment segment;
		float dist;
		Vector2 closest_point;
		bool valid = chunk->get_distance_to_closest_road_segment(p_positions[i], segment, &dist, &closest_point);

		out_ptrw[i] = {
			.from = segment.start,
			.to = segment.end,
			.closest_point = closest_point,
			.distance = dist,
			.valid = valid,
		};
	}

	return out;
}

void TerrainRoadConnectionChunk::generate() {
	FuncProfile;
	Rect2 check_rect = get_chunk_bounds().grow(connection_radius);
	Vector<Vector2> points = layer->get_points_in_bounds(check_rect);
	const float check_dist_squared = connection_radius * connection_radius;

	Vector<Vector2> points_copy = points.duplicate();
	for (int i = 0; i < points.size(); i++) {
		//Vector<Vector2> points_in_radius = layer->get_points_in_radius(points[i], connection_radius);
		sort_by_dist_to_point(points[i], points_copy);
		int added_points = 0;
		for (int j = 0; j < points_copy.size(); j++) {
			const float dist_sq = points_copy[j].distance_squared_to(points[i]);
			if (Math::is_zero_approx(dist_sq)) {
				continue;
			}
			if (dist_sq < check_dist_squared) {
				added_points++;
				connections.push_back({
						.from = points[i],
						.to = points_copy[j],
				});
			}
			if (added_points > maximum_connections) {
				break;
			}
		}
	}

	LocalVector<SegmentQuadtree::QuadTreeSegment> base_segments;

	for (int i = 0; i < connections.size(); i++) {
		base_segments.push_back({ .start = connections[i].from, .end = connections[i].to });
	}

	for (int i = 0; i < connections.size(); i++) {
		/*Ref<AStarGrid2D> astar_grid;
		astar_grid.instantiate();
		Vector2 diff = connections[i].to - connections[i].from;
		int max_axis = Math::ceil(diff[diff.max_axis_index()]);
		astar_grid->set_region(Rect2i(Vector2i(connections[i].from), Vector2i(max_axis+1, max_axis+1)));
		astar_grid->set_cell_size(Vector2(64, 64));
		astar_grid->update();
		astar_grid->get_id_path(connections[i].from, connections[i].to);*/
	}

	quad_tree.initialize(get_chunk_bounds().grow(4096), base_segments);
}

void TerrainRoadConnectionChunk::debug_draw(ChunkinatorDebugDrawer *p_debug_drawer) const {
	if (get_chunk_index() != Vector2i(0, 0)) {
		return;
	}

	for (int i = 0; i < connections.size(); i++) {
		p_debug_drawer->draw_line(connections[i].from, connections[i].to);
	}
}
bool TerrainRoadConnectionChunk::get_distance_to_closest_road_segment(Vector2 p_position, SegmentQuadtree::QuadTreeSegment &r_segment, float *r_distance, Vector2 *r_closest_point) const {
	int segment = quad_tree.find_closest_segment(p_position, 300, r_distance, r_closest_point);
	if (segment != -1) {
		r_segment = quad_tree.get_segment(segment);
		return true;
	}

	return false;
}

Ref<Image> TerrainRoadConnectionChunk::get_heightmap() const {
	return nullptr;
}
