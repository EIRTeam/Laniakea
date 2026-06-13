#pragma once

#include "geometry.h"
#include "godot_cpp/classes/control.hpp"
#include "godot_cpp/classes/geometry2d.hpp"
#include "godot_cpp/classes/node.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/templates/local_vector.hpp"
#include "godot_cpp/variant/rect2i.hpp"
#include "godot_cpp/variant/vector2.hpp"

#include <limits>
#include <optional>
#include <queue>

using namespace godot;

class SegmentQuadTreeDebug;

class SegmentQuadtree {
	static constexpr int TREE_MAX_DEPTH = 4;

	Rect2 tree_world_rect;

	enum NodePosition {
		POSITION_NW,
		POSITION_NE,
		POSITION_SW,
		POSITION_SE,
		POSITION_MAX
	};

	struct QuadTreeNode {
		int children_begin = -1;
		int indices_begin = -1;
		int indices_count = -1;
		int depth = 0;
		int parent = -1;
		Rect2i rect;
	};

public:
	struct QuadTreeSegment {
		Vector2 start;
		Vector2 end;
	};

	struct SegmentQueryResult {
		float distance;
		Vector2 start;
		Vector2 end;
	};

private:
	LocalVector<QuadTreeNode> nodes;
	LocalVector<QuadTreeSegment> segments;
	LocalVector<int> node_segment_indices;

	Rect2i calculate_rect_for_pos(const Rect2i &p_parent_rect, NodePosition p_pos);

	void _insert_child_segments(const int p_node_idx, const LocalVector<int> &p_segments);

public:
	void initialize(Rect2i p_rect, const LocalVector<QuadTreeSegment> &p_segments);

	int find_closest_segment(const Vector2 &p_point, std::optional<float> p_max_radius, float *r_distance = nullptr, Vector2 *r_closest_point = nullptr) const;

	QuadTreeSegment get_segment(int p_segment_idx) const;
	friend class SegmentQuadTreeDebug;
};

class SegmentQuadTreeDebug : public Control {
	GDCLASS(SegmentQuadTreeDebug, Control);
	SegmentQuadtree tree;

	void _notification(int p_what);
	Transform2D get_draw_trf() const;

	Vector2 test_point = Vector2(1, 1);

	static void _bind_methods();

public:
	SegmentQuadTreeDebug();
};
