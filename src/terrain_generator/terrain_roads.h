#pragma once

#include "chunkinator/chunkinator_chunk.h"
#include "chunkinator/chunkinator_layer.h"
#include "godot_cpp/classes/fast_noise_lite.hpp"
#include "godot_cpp/classes/image.hpp"
#include "godot_cpp/classes/image_texture.hpp"
#include "segment_quadtree.h"

class TerrainRoadConnectionLayer : public ChunkinatorLayer {
	virtual int get_chunk_size() const override;
	virtual Ref<ChunkinatorChunk> instantiate_chunk() override;

public:
	Vector<Vector2> get_points_in_radius(Vector2 p_position, float p_radius) const;
	Vector<Vector2> get_points_in_bounds(Rect2 p_bounds) const;
	double get_height_at_position(Vector2 p_position) const;
	bool get_distance_to_closest_road_segment(const Vector2 &p_position, SegmentQuadtree::QuadTreeSegment &r_segment, float *r_distance = nullptr, Vector2 *r_closest_point = nullptr) const;
	struct SegmentQueryResult {
		Vector2 from;
		Vector2 to;
		Vector2 closest_point;
		float distance = 0.0f;
		bool valid = false;
	};
	Vector<TerrainRoadConnectionLayer::SegmentQueryResult> get_distance_to_closest_road_segments_batched(const Vector<Vector2> &p_positions) const;
};

class TerrainRoadConnectionChunk : public ChunkinatorChunk {
public:
	TerrainRoadConnectionLayer *layer = nullptr;
	struct Connection {
		Vector2 from;
		Vector2 to;
	};
	SegmentQuadtree quad_tree;
	LocalVector<Connection> connections;
	const float connection_radius = 9500.0f;
	const int maximum_connections = 3;
	const float ROAD_WIDTH = 3.0f;
	virtual void generate() override;
	virtual void debug_draw(ChunkinatorDebugDrawer *p_debug_drawer) const override;
	bool get_distance_to_closest_road_segment(Vector2 p_position, SegmentQuadtree::QuadTreeSegment &r_segment, float *r_distance = nullptr, Vector2 *r_closest_point = nullptr) const;
	Ref<Image> get_heightmap() const;
	friend class TerrainRoadConnectionLayer;
};
