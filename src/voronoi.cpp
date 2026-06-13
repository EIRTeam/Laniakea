#include "voronoi.h"

#include "godot_cpp/classes/geometry2d.hpp"
#include "godot_cpp/variant/packed_vector2_array.hpp"
#include "godot_cpp/variant/rect2i.hpp"

Voronoi::Voronoi(Rect2i p_rect, float p_distance_between_points) :
		rect(p_rect), distance_between_points(p_distance_between_points) {
}

void Voronoi::generate_points() {
	first_point = rect.position / distance_between_points;
	last_point = (rect.position + rect.size) / distance_between_points;

	data_size = last_point - first_point;

	points.resize(data_size.x * data_size.y);

	VoronoiPoint *points_w = points.ptrw();

	const float half_dist_between_points = (distance_between_points / 2.0);

	PackedVector2Array delaunay_arr;
	delaunay_arr.resize(points.size());
	Vector2 *delaunay_arr_w = delaunay_arr.ptrw();

	for (int y = 0; y < data_size.y; y++) {
		for (int x = 0; x < data_size.x; x++) {
			Vector2i grid_point = Vector2(first_point + Vector2i(x, y));
			Vector2 deviation = Vector2(noise_x->get_noise_2dv(grid_point) * 0.5, noise_y->get_noise_2dv(grid_point) * 0.5);
			Vector2 point = (grid_point + deviation) * half_dist_between_points;
			const int idx = y * data_size.x + x;
			points_w[idx].position = point;
			delaunay_arr_w[idx] = point;
		}
	}

	delaunay = Geometry2D::get_singleton()->triangulate_delaunay(delaunay_arr);

	auto push_triangle = [points_w](int p_triangle_idx, int p_position_idx) {
		DEV_ASSERT(points_w[p_position_idx].triangle_count < MAX_TRIANGLES_PER_POINT);
		VoronoiPoint *point = &points_w[p_position_idx];
		points_w[p_position_idx].triangles[++(point->triangle_count)] = p_triangle_idx;
	};

	for (int i = 0; i < delaunay.size() / 3; i++) {
		push_triangle(i, delaunay[i * 3]);
		push_triangle(i, delaunay[i * 3 + 1]);
		push_triangle(i, delaunay[i * 3 + 2]);
	}
}

Voronoi::QueryVoronoiDiagramResult Voronoi::query(Vector2 p_point) {
	Vector2i center_i;
	center_i.x = p_point.x / distance_between_points;
	center_i.y = p_point.y / distance_between_points;

	QueryVoronoiDiagramResult result = {
		.valid = false
	};

	if (center_i.x <= first_point.x || center_i.y <= first_point.y || center_i.x >= last_point.x || center_i.y >= last_point.y) {
		print_error("Out of bounds access!!");
		return result;
	}

	for (int y = center_i.y - 1; y <= center_i.y + 1; y++) {
		for (int x = center_i.x - 1; y <= center_i.x + 1; x++) {
			const VoronoiPoint &point = points[x + y * data_size.y];
			for (int tri = 0; tri < point.triangle_count; tri++) {
				const int delaunay_idx = point.triangles[tri] * 3;
				const Vector2 &a = points[delaunay[delaunay_idx]].position;
				const Vector2 &b = points[delaunay[delaunay_idx + 1]].position;
				const Vector2 &c = points[delaunay[delaunay_idx + 2]].position;
				const bool is_inside = Geometry2D::get_singleton()->point_is_inside_triangle(p_point, a, b, c);

				if (is_inside) {
					result.valid = true;
					result.closest_point_indices[0] = delaunay[delaunay_idx];
					result.closest_point_indices[1] = delaunay[delaunay_idx + 1];
					result.closest_point_indices[2] = delaunay[delaunay_idx + 2];
				}
				return result;
			}
		}
	}

	return result;
}

const Vector<Voronoi::VoronoiPoint> &Voronoi::get_points() const {
	return points;
}
