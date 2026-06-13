#pragma once

#include "godot_cpp/classes/fast_noise_lite.hpp"
#include "godot_cpp/variant/rect2i.hpp"

using namespace godot;

class Voronoi {
	int distance_between_points = 10;
	float noise_frequency = 0.01f;
	Ref<FastNoiseLite> noise_x;
	Ref<FastNoiseLite> noise_y;
	Vector2i first_point;
	Vector2i last_point;
	Size2i data_size;
	static constexpr int MAX_TRIANGLES_PER_POINT = 4;
	struct VoronoiPoint {
		Vector2 position;
		int triangles[MAX_TRIANGLES_PER_POINT];
		int triangle_count = 0;
	};

	Vector<VoronoiPoint> points;
	PackedInt32Array delaunay;

	Rect2i rect;

public:
	Voronoi(Rect2i p_rect, float p_distance_between_points);
	void generate_points();

	struct QueryVoronoiDiagramResult {
		bool valid = false;
		int closest_point_indices[3];
	};

	QueryVoronoiDiagramResult query(Vector2 p_point);

	const Vector<VoronoiPoint> &get_points() const;
};
